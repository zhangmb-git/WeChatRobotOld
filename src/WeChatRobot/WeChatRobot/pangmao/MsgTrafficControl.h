#pragma once

#include <atomic>
#include <string>

class CMsgTrafficControl {
  public:

    static void SetLimit(int intervalMs, int limit);


    //************************************
    // Method:    Grant
    // FullName:  CMsgTrafficControl::Grant
    // Access:    public static
    // Returns:   bool
    // Qualifier:
    // Parameter: bool & out_is_first:是否第一次出发流控
    //************************************
    static bool Grant(bool& out_can_i_need_reply, std::string& out_random_answer);
};

