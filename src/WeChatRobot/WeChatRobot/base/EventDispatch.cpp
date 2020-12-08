#include "EventDispatch.h"
#include "BaseSocket.h"
#include "util.h"
#include <list>
#include "ZLogger.h"

#define MIN_TIMER_DURATION    100    // 100 miliseconds

CEventDispatch *CEventDispatch::m_pEventDispatch = NULL;

CEventDispatch::CEventDispatch() {
    running = false;
#ifdef _WIN32
    FD_ZERO(&m_read_set);
    FD_ZERO(&m_write_set);
    FD_ZERO(&m_excep_set);
#elif __APPLE__
    m_kqfd = kqueue();
    if (m_kqfd == -1) {
        LogError("kqueue failed");
    }
#else

    if((max_files = sysconf(_SC_OPEN_MAX)) < 0){
        log("Get the maximum number of files that a process can have open at any time failed\n");
        max_files = 1048576;
    } else
        log("The maximum number of files that a process can have open at any time is %ld\n", max_files);

    max_files /= 2;
    m_epfd = epoll_create(max_files);
    if (m_epfd == -1)
    {
        log("epoll_create failed");
    }
#endif
}

CEventDispatch::~CEventDispatch() {
#ifdef _WIN32

#elif __APPLE__
    close(m_kqfd);
#else
    close(m_epfd);
#endif
}

#if 0
void CEventDispatch::AddTimer(callback_t callback, void* user_data, uint64_t interval)
{
    list<TimerItem*>::iterator it;
    for (it = m_timer_list.begin(); it != m_timer_list.end(); it++)
    {
        TimerItem* pItem = *it;
        if (pItem->callback == callback && pItem->user_data == user_data)
        {
            pItem->interval = interval;
            pItem->next_tick = get_tick_count() + interval;
            return;
        }
    }

    TimerItem* pItem = new TimerItem;
    pItem->callback = callback;
    pItem->user_data = user_data;
    pItem->interval = interval;
    pItem->next_tick = get_tick_count() + interval;
    m_timer_list.push_back(pItem);
}
#endif //0


void CEventDispatch::EventTimerInit() {
    memset(&m_event_timer_sentinel, 0, sizeof(m_event_timer_sentinel));
    rbtree_init(&m_event_timer_rbtree, &m_event_timer_sentinel,
                rbtree_insert_timer_value);
}


void CEventDispatch::AddTimer(rb_timer_item *pEventItem) {
    if (NULL == pEventItem)
        return;

    if (pEventItem->timer_set) {
        // It have already set timer
        RemoveTimer(pEventItem);
    }

    uintptr_t nKey = get_tick_count() + pEventItem->interval;
    pEventItem->timer_tree.key = nKey;
    rbtree_insert(&m_event_timer_rbtree, &pEventItem->timer_tree);
    pEventItem->timer_set = 1;
}


void CEventDispatch::CancelTimers(void) {
    rb_timer_item *ev;
    rbtree_node_t *node, *root, *sentinel;

    sentinel = m_event_timer_rbtree.sentinel;

    for (;;) {
        root = m_event_timer_rbtree.root;

        if (root == sentinel) {
            return;
        }

        node = rbtree_min(root, sentinel);

        ev = (rb_timer_item *) ((char *) node - offsetof(rb_timer_item, timer_tree));

        rbtree_delete(&m_event_timer_rbtree, &ev->timer_tree);

        ev->timer_tree.left = NULL;
        ev->timer_tree.right = NULL;
        ev->timer_tree.parent = NULL;

        ev->timer_set = 0;
        ev->callback(ev->user_data, NETLIB_MSG_TIMER, 0, NULL);
    }
}

void CEventDispatch::CheckTimer() {
    EventExpireTimers();
}

void CEventDispatch::RemoveTimer(rb_timer_item *pEventItem) {
    if (NULL == pEventItem)
        return;

    if (pEventItem->timer_set == 1) {
        rbtree_delete(&m_event_timer_rbtree, &pEventItem->timer_tree);
        pEventItem->timer_set = 0;
    }
}

void CEventDispatch::DisableTimer(rb_timer_item *pEventItem) {
    if (NULL == pEventItem)
        return;

    pEventItem->closed = 1;
}


void CEventDispatch::EventExpireTimers(void) {
    rb_timer_item *ev;
    rbtree_node_t *node, *root, *sentinel;
    sentinel = m_event_timer_rbtree.sentinel;

    for (;;) {
        root = m_event_timer_rbtree.root;

        if (root == sentinel) {
            return;
        }//End if

        node = rbtree_min(root, sentinel);

        uintptr_t current_msec = get_tick_count();
        /* node->key > current_time, no overtimer*/
        if ((intptr_t) (node->key - current_msec) > 0) {
            return;
        }

        ev = (rb_timer_item *) ((char *) node - offsetof(rb_timer_item, timer_tree));
        rbtree_delete(&m_event_timer_rbtree, &ev->timer_tree);
        ev->timer_set = 0;
        ev->callback(ev->user_data, NETLIB_MSG_TIMER, 0, NULL);
        if (ev->closed == 0) {
            AddTimer(ev);
            ev->timer_set = 1;
		}
		else {
			//DEBUG("###Remove timer %d", ev->interval);
		}
    }// End for
}

#if 0
void CEventDispatch::RemoveTimer(callback_t callback, void* user_data)
{
    list<TimerItem*>::iterator it;
    for (it = m_timer_list.begin(); it != m_timer_list.end(); it++)
    {
        TimerItem* pItem = *it;
        if (pItem->callback == callback && pItem->user_data == user_data)
        {
            m_timer_list.erase(it);
            delete pItem;
            return;
        }
    }
}
#endif //0

void CEventDispatch::_CheckTimer() {
    uint64_t curr_tick = get_tick_count();
    std::list<TimerItem *>::iterator it;

    for (it = m_timer_list.begin(); it != m_timer_list.end();) {
        TimerItem *pItem = *it;
        it++;        // iterator maybe deleted in the callback, so we should increment it before callback
        if (curr_tick >= pItem->next_tick) {
            pItem->next_tick += pItem->interval;
            pItem->callback(pItem->user_data, NETLIB_MSG_TIMER, 0, NULL);
        }
    }
}

void CEventDispatch::AddLoop(callback_t callback, void *user_data) {
    TimerItem *pItem = new TimerItem;
    pItem->callback = callback;
    pItem->user_data = user_data;
    m_loop_list.push_back(pItem);
}

void CEventDispatch::_CheckLoop() {
    for (std::list<TimerItem *>::iterator it = m_loop_list.begin(); it != m_loop_list.end(); it++) {
        TimerItem *pItem = *it;
        pItem->callback(pItem->user_data, NETLIB_MSG_LOOP, 0, NULL);
    }
}

CEventDispatch *CEventDispatch::Instance() {
    if (m_pEventDispatch == NULL) {
        m_pEventDispatch = new CEventDispatch();
    }

    return m_pEventDispatch;
}

#ifdef _WIN32

void CEventDispatch::AddEvent(CBaseSocket &socket, uint8_t socket_event)
{
	std::lock_guard<std::mutex>  lock(m_mutex);

    if ((socket_event & SOCKET_READ) != 0)
    {
        FD_SET(socket.GetSocket(), &m_read_set);
    }

    if ((socket_event & SOCKET_WRITE) != 0)
    {
        FD_SET(socket.GetSocket(), &m_write_set);
    }

    if ((socket_event & SOCKET_EXCEP) != 0)
    {
        FD_SET(socket.GetSocket(), &m_excep_set);
    }
}

void CEventDispatch::RemoveEvent(CBaseSocket &socket, uint8_t socket_event)
{
	std::lock_guard<std::mutex>  lock(m_mutex);

    if ((socket_event & SOCKET_READ) != 0)
    {
        FD_CLR(socket.GetSocket(), &m_read_set);
    }

    if ((socket_event & SOCKET_WRITE) != 0)
    {
        FD_CLR(socket.GetSocket(), &m_write_set);
    }

    if ((socket_event & SOCKET_EXCEP) != 0)
    {
        FD_CLR(socket.GetSocket(), &m_excep_set);
    }

	if ((socket_event & SOCKET_EXCEP) != 0)
	{
		FD_CLR(socket.GetSocket(), &m_excep_set);
	}

}

void CEventDispatch::StartDispatch(uint32_t wait_timeout)
{
    fd_set read_set, write_set, excep_set;
    timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = wait_timeout * 1000;	// 10 millisecond

    if(running)
        return;
    running = true;

    while (running)
    {
        _CheckTimer();
        _CheckLoop();

        if (!m_read_set.fd_count && !m_write_set.fd_count && !m_excep_set.fd_count)
        {
            Sleep(MIN_TIMER_DURATION);
            continue;
        }
		{
			std::lock_guard<std::mutex> lock(m_mutex);
			memcpy(&read_set, &m_read_set, sizeof(fd_set));
			memcpy(&write_set, &m_write_set, sizeof(fd_set));
			memcpy(&excep_set, &m_excep_set, sizeof(fd_set));
		}
        
        int nfds = select(0, &read_set, &write_set, &excep_set, &timeout);

        if (nfds == SOCKET_ERROR)
        {
            LogError("select failed, error code: {}", GetLastError());
            Sleep(MIN_TIMER_DURATION);
            continue;			// select again
        }

        if (nfds == 0)
        {
            continue;
        }

        for (u_int i = 0; i < read_set.fd_count; i++)
        {
            //log("select return read count=%d\n", read_set.fd_count);
            SOCKET fd = read_set.fd_array[i];
            CBaseSocket* pSocket = FindBaseSocket((net_handle_t)fd);
            if (pSocket)
            {
                pSocket->OnRead();
                //pSocket->ReleaseRef();
            }
        }

        for (u_int i = 0; i < write_set.fd_count; i++)
        {
            //log("select return write count=%d\n", write_set.fd_count);
            SOCKET fd = write_set.fd_array[i];
            CBaseSocket* pSocket = FindBaseSocket((net_handle_t)fd);
            if (pSocket)
            {
                pSocket->OnWrite();
                //pSocket->ReleaseRef();
            }
        }

        for (u_int i = 0; i < excep_set.fd_count; i++)
        {
            //log("select return exception count=%d\n", excep_set.fd_count);
            SOCKET fd = excep_set.fd_array[i];
            CBaseSocket* pSocket = FindBaseSocket((net_handle_t)fd);
            if (pSocket)
            {
                pSocket->OnClose();
                //pSocket->ReleaseRef();
            }
        }

    }
}

void CEventDispatch::StopDispatch()
{
    running = false;
}

#elif __APPLE__

void CEventDispatch::AddEvent(CBaseSocket &socket, uint8_t socket_event) {
    struct kevent ke;

    if ((socket_event & SOCKET_READ) != 0) {
        EV_SET(&ke, socket.GetSocket(), EVFILT_READ, EV_ADD, 0, 0, NULL);
        kevent(m_kqfd, &ke, 1, NULL, 0, NULL);
    }

    if ((socket_event & SOCKET_WRITE) != 0) {
        EV_SET(&ke, socket.GetSocket(), EVFILT_WRITE, EV_ADD, 0, 0, NULL);
        kevent(m_kqfd, &ke, 1, NULL, 0, NULL);
    }
}

void CEventDispatch::RemoveEvent(CBaseSocket &socket, uint8_t socket_event) {
    struct kevent ke;

    if ((socket_event & SOCKET_READ) != 0) {
        EV_SET(&ke, socket.GetSocket(), EVFILT_READ, EV_DELETE, 0, 0, NULL);
        kevent(m_kqfd, &ke, 1, NULL, 0, NULL);
    }

    if ((socket_event & SOCKET_WRITE) != 0) {
        EV_SET(&ke, socket.GetSocket(), EVFILT_WRITE, EV_DELETE, 0, 0, NULL);
        kevent(m_kqfd, &ke, 1, NULL, 0, NULL);
    }
}

void CEventDispatch::StartDispatch(uint32_t wait_timeout) {
    struct kevent events[1024];
    int nfds = 0;
    struct timespec timeout;
    timeout.tv_sec = 0;
    timeout.tv_nsec = wait_timeout * 1000000;

    if (running)
        return;
    running = true;

    while (running) {
        nfds = kevent(m_kqfd, NULL, 0, events, 1024, &timeout);

        for (int i = 0; i < nfds; i++) {
            int ev_fd = events[i].ident;
            CBaseSocket *pSocket = FindBaseSocket(ev_fd);
            if (!pSocket)
                continue;

            if (events[i].filter == EVFILT_READ) {
                //log("OnRead, socket=%d\n", ev_fd);
                pSocket->OnRead();
            }

            if (events[i].filter == EVFILT_WRITE) {
                //log("OnWrite, socket=%d\n", ev_fd);
                pSocket->OnWrite();
            }

            //pSocket->ReleaseRef();
        }

        //_CheckTimer();
        _CheckLoop();
        CheckTimer();
    }
}

void CEventDispatch::StopDispatch() {
    running = false;
}

#else

void CEventDispatch::AddEvent(CBaseSocket &socket, uint8_t socket_event)
{
    int			op;
    uint32_t		events, prev;
    struct epoll_event	ee;
    bool			active = false;
    int			fd;

    if (socket_event == SOCKET_READ) {
        active = socket.getWriteActive();
        prev = EPOLLOUT;
        events = EPOLLIN|EPOLLRDHUP;
    } else if (socket_event == SOCKET_WRITE) {
        active = socket.getReadActive();
        prev = EPOLLIN|EPOLLRDHUP;
        events = EPOLLOUT;
    } else {
        loge("AddEvent: socket_event must be read or write");
        return;
    }

    if (active) {
        op = EPOLL_CTL_MOD;
        events |= prev;
    } else {
        op = EPOLL_CTL_ADD;
    }

    // to fix valgrind error:Syscall param epoll_ctl(event) points to uninitialised byte(s)
    memset(&ee.data, 0, sizeof(ee.data));
    ee.events = events | EPOLLET; // Edge trigger
    fd = socket.GetSocket();
    ee.data.fd = fd;
    if (epoll_ctl(m_epfd, op, fd, &ee) == -1) {
        log("epoll_ctl(%d, %d) failed", op, fd);
        return;
    }
    if (socket_event == SOCKET_READ) {
        socket.setReadActive(true);
    } else {
        socket.setWriteActive(true);
    }
}

void CEventDispatch::RemoveEvent(CBaseSocket &socket, uint8_t socket_event)
{
    int			op;
    uint32_t		prev;
    struct epoll_event	ee;
    bool			active = false;
    int			fd;

    if (socket_event == SOCKET_CLOSE) {
        /* when the file descriptor is closed, the epoll automatically deletes
         * it from its queue, so we do not need to delete explicitly the event
         * before the closing the file descriptor
         */
        return;
    } else if (socket_event == SOCKET_READ) {
        active = socket.getWriteActive();
        prev = EPOLLOUT;
    } else if (socket_event == SOCKET_WRITE) {
        active = socket.getReadActive();
        prev = EPOLLIN|EPOLLRDHUP;
    } else {
        loge("RemoveEvent: socket_event must be read or write");
        return;
    }

    fd = socket.GetSocket();
    if (active) {
        op = EPOLL_CTL_MOD;
        ee.events = prev | EPOLLET; // Edge trigger
        ee.data.fd = fd;
    } else {
        op = EPOLL_CTL_DEL;
        ee.events = 0;
        ee.data.ptr = NULL;
    }

    if (epoll_ctl(m_epfd, op, fd, &ee) == -1) {
        loge("epoll_ctl(%d, %d) failed", op, fd);
        return;
    }

    if (socket_event == SOCKET_READ) {
        socket.setReadActive(false);
    } else {
        socket.setWriteActive(false);
    }
}

void CEventDispatch::StartDispatch(uint32_t wait_timeout)
{
    struct epoll_event events[max_files];
    int nfds = 0;

    if(running)
        return;
    running = true;

    while (running)
    {
        nfds = epoll_wait(m_epfd, events, max_files, wait_timeout);
        for (int i = 0; i < nfds; i++)
        {
            int ev_fd = events[i].data.fd;
            CBaseSocket* pSocket = FindBaseSocket(ev_fd);
            if (!pSocket)
                continue;

            //Commit by zhfu @2015-02-28
#ifdef EPOLLRDHUP
            if (events[i].events & EPOLLRDHUP)
            {
                DEBUG("On Peer Close, socket=%d", ev_fd);
                pSocket->OnClose();
            }
#endif
            // Commit End

            if (events[i].events & EPOLLIN)
            {
                //log("OnRead, socket=%d\n", ev_fd);
                pSocket->OnRead();
            }

            if (events[i].events & EPOLLOUT)
            {
                //log("OnWrite, socket=%d\n", ev_fd);
                pSocket->OnWrite();
            }

            if (events[i].events & (EPOLLPRI | EPOLLERR | EPOLLHUP))
            {
                DEBUG("OnClose, socket=%d\n", ev_fd);
                pSocket->OnClose();
            }

            //pSocket->ReleaseRef();
        }

        //_CheckTimer();
        _CheckLoop();
        CheckTimer();
    }
}

void CEventDispatch::StopDispatch()
{
    running = false;
}


int  CEventDispatch::TimerTreeNodeCount()
{
    return count_tree_node(m_event_timer_rbtree.root,m_event_timer_rbtree.sentinel);
}

int  CEventDispatch::TimerTreeHight()
{
    return hight_tree(m_event_timer_rbtree.root,m_event_timer_rbtree.sentinel);
}

#endif