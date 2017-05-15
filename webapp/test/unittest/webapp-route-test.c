#include "ctest.h"
#include "webapp-route.h"

TEST(route, add_route)
{
    route_t* r = route_new(NULL);

    route_add(r, "/user/gordon", (webapp_handler_t)1, 0);
    route_add(r, "/user/you", (webapp_handler_t)1, 0);
    route_add(r, "/user/gordon/profile", (webapp_handler_t)1, 0);
    route_add(r, "/user", (webapp_handler_t)1, 0);

    route_destroy(r);
}