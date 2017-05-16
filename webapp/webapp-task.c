#include "webapp-task.h"

int webapp_task_read(webapp_thread_t* t)
{
    webapp_t* web = t->web;
    int rv = epoll_wait(t->epfd, t->evts, t->nfd, 100);
    if (rv == 0) {
    } else if (rv < 0) {
        //if (t->http->sig_term || t->http->sig_usr1 || t->http->sig_usr2) {
        //	continue;
        //}
        debug("epoll_wait error, errno=%d\n", errno);
        return -1;
    } else {
        for (int i = 0; i < rv; i++) {
            struct epoll_event* ev = &(t->evts[i]);
            int fd = (int)ev->data.ptr;
            connection_t* con = &(web->conns[fd]);

            if (con->fd == t->fd[0]) {
                intptr_t ptr = 0;
                if (read(con->fd, &ptr, sizeof(intptr_t)) != sizeof(intptr_t)) {
                    debug("pipe read failed.\n");
                    continue;
                }
                int fd = (int)ptr;
                debug("thread accept: fd=%d\n", fd);

                tcp_nodelay(fd, true);
                tcp_noblock(fd, true);

                con = &(web->conns[fd]);
                con->fd = fd;

                if (epoll_add_fd(t->epfd, EPOLLIN, fd) < 0) {
                    debug("epoll_add_fd failed.\n");
                    close(fd);
                    continue;
                }
            } else {
                // TODO read connection
                webapp_list_node_t* n = calloc(1, sizeof(*n));
                n->data = con;
                list_add_tail(&(t->read_queue), &(n->node));
            }
        }
    }
    return 0;
}

int webapp_task_process(webapp_thread_t* t)
{
    webapp_list_node_t* item = NULL;
    list_for_each(&t->read_queue, item, node) {
        connection_t* con = (connection_t*)item->data;
        debug("fd(%d) receive data\n", con->fd);
    }
    return 0;
}

int webapp_task_write(webapp_thread_t* t)
{
    webapp_list_node_t* item = NULL;
    list_for_each(&t->write_queue, item, node) {
        connection_t* con = (connection_t*)item->data;
        debug("fd(%d) write data\n", con->fd);
    }
    return 0;
}
