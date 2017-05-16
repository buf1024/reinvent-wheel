#include "ctest.h"
#include "webapp-route.h"

TEST(route, add_route)
{
    route_t* r = route_new(NULL);

    
    route_add(r, "/:ab", (webapp_handler_t)1, 0);
    route_add(r, "/ab/", (webapp_handler_t)2, 0);
    route_add(r, "/", (webapp_handler_t)3, 0);
    route_add(r, "/user/you", (webapp_handler_t)4, 0);
    route_add(r, "/user/:gordon/profile", (webapp_handler_t)5, 0);
    route_add(r, "/user/gordon", (webapp_handler_t)6, 0);
    route_add(r, "/user/t/:csdn", (webapp_handler_t)7, 0);
    route_add(r, "/user", (webapp_handler_t)8, 0);
    route_add(r, "/hello/*name", (webapp_handler_t)9, 0);
    route_add(r, "/hello/abc", (webapp_handler_t)10, 0);

    route_walk(r);

    route_info_t* info = NULL;
    info = route_get(r, "/user");
    ASSERT_TRUE(info != NULL);
    ASSERT_EQ((webapp_handler_t)8, info->handlers[0]);
    ASSERT_EQ(0, info->params_num);
    route_info_free(info);

    info = route_get(r, "/user/gordon/profile");
    ASSERT_TRUE(info == NULL);

    info = route_get(r, "/user/you");
    ASSERT_TRUE(info != NULL);
    ASSERT_EQ((webapp_handler_t)4, info->handlers[0]);
    ASSERT_EQ(0, info->params_num);
    route_info_free(info);

    info = route_get(r, "/user/gordon");
    ASSERT_TRUE(info != NULL);
    ASSERT_EQ((webapp_handler_t)6, info->handlers[0]);
    ASSERT_EQ(0, info->params_num);
    route_info_free(info);

    info = route_get(r, "/");
    ASSERT_TRUE(info != NULL);
    ASSERT_EQ((webapp_handler_t)3, info->handlers[0]);
    ASSERT_EQ(0, info->params_num);
    route_info_free(info);

    info = route_get(r, "/a");
    ASSERT_TRUE(info != NULL);
    ASSERT_EQ((webapp_handler_t)1, info->handlers[0]);
    ASSERT_EQ(1, info->params_num);
    ASSERT_STREQ("ab", info->params[0].key);
    ASSERT_STREQ("a", info->params[0].val);
    route_info_free(info);

    info = route_get(r, "/hello/gordon");
    ASSERT_TRUE(info != NULL);
    ASSERT_EQ((webapp_handler_t)9, info->handlers[0]);
    ASSERT_EQ(1, info->params_num);
    ASSERT_STREQ("name", info->params[0].key);
    ASSERT_STREQ("gordon", info->params[0].val);
    route_info_free(info);

    info = route_get(r, "/hello/gordon/yaa/aaffy");
    ASSERT_TRUE(info != NULL);
    ASSERT_EQ((webapp_handler_t)9, info->handlers[0]);
    ASSERT_EQ(1, info->params_num);
    ASSERT_STREQ("name", info->params[0].key);
    ASSERT_STREQ("gordon/yaa/aaffy", info->params[0].val);
    route_info_free(info);

    route_destroy(r);
}