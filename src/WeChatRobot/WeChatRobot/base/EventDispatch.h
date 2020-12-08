/*
 * A socket event dispatcher, features include: 
 * 1. portable: worked both on Windows, MAC OS X,  LINUX platform
 * 2. a singleton pattern: only one instance of this class can exist
 */
#ifndef __EVENT_DISPATCH_H__
#define __EVENT_DISPATCH_H__

#include <list>
#include <mutex>

#include "ostype.h"
#include "time.h"
#include "rbtree.h"
#include "BaseSocket.h"

enum {
	SOCKET_READ		= 0x1,
	SOCKET_WRITE		= 0x2,
	SOCKET_EXCEP		= 0x4,
	SOCKET_ALL		= 0x7,
	SOCKET_CLOSE		= 0x9,
};

typedef struct {
        callback_t      callback;
        void*           user_data;
        uint64_t        interval;
        //uint64_t        next_tick;
        rbtree_node_t   timer_tree;
        unsigned char   timer_set;
        unsigned char   closed;
} rb_timer_item;





#define rbtimer_init(rbtimer,callback_fun, callback_data, interval_value,timer_set_value,closed_value)                                           \
    (rbtimer)->callback = callback_fun;                            \
    (rbtimer)->user_data = callback_data;                          \
    (rbtimer)->interval = interval_value;										\
    (rbtimer)->timer_set = timer_set_value;									\
    (rbtimer)->closed = closed_value


class CEventDispatch
{
public:
	virtual ~CEventDispatch();

	void AddEvent(CBaseSocket &socket, uint8_t socket_event);
	void RemoveEvent(CBaseSocket &socket, uint8_t socket_event);

	//void AddTimer(callback_t callback, void* user_data, uint64_t interval);
	//void RemoveTimer(callback_t callback, void* user_data);

	void EventTimerInit();
	void AddTimer(rb_timer_item* pEventItem);
	void RemoveTimer(rb_timer_item* pEventItem);
	void DisableTimer(rb_timer_item* pEventItem);
	void CancelTimers(void);
	void CheckTimer();
	void EventExpireTimers(void);
	int  TimerTreeNodeCount();
	int  TimerTreeHight();
    
    void AddLoop(callback_t callback, void* user_data);

	void StartDispatch(uint32_t wait_timeout = 100);
    void StopDispatch();
    
    bool isRunning() {return running;}

	static CEventDispatch* Instance();
protected:
	CEventDispatch();

private:
	void _CheckTimer();
    void _CheckLoop();

	typedef struct {
		callback_t	callback;
		void*		user_data;
		uint64_t	interval;
		uint64_t	next_tick;
	} TimerItem;

private:
#ifdef _WIN32
	fd_set	m_read_set;
	fd_set	m_write_set;
	fd_set	m_excep_set;
#elif __APPLE__
	int 	m_kqfd;
#else
	int		m_epfd;
#endif
	std::mutex	 m_mutex;
	std::list<TimerItem*>	m_timer_list;
	std::list<TimerItem*>	m_loop_list;

	static CEventDispatch* m_pEventDispatch;
    
    bool running;
    long max_files;
	rbtree_t       m_event_timer_rbtree;
	rbtree_node_t  m_event_timer_sentinel;
};

#endif
