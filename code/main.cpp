/*
 * @Author       : 晚乔最美
 * @Date         : 2023-06-18
 * @copyleft Apache 2.0
 */ 
#include <unistd.h>
#include <string>
#include "server/webserver.h"

int main() {

    WebServer server(
        1316, 3, 60000, false,             /* 端口 ET模式 timeoutMs 优雅退出  */
        3306, "root", "root", "webserver", /* Mysql配置 */
        12, 4, true, 1, 1024);             /* 连接池数量 线程池数量 日志开关 日志等级 日志异步队列容量 */
    server.Start();
} 
  