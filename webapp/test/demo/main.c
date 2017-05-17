#include <stdio.h>
#include <stdlib.h>

#include "webapp.h"

void handler(webapp_context_t* ctx)
{

}

int main()
{
    webapp_t* web = webapp_new();

    GET(web, "/get", handler);
    POST(web, "/post", handler);
    PUT(web, "/put", handler);
    DELETE(web, "/delete", handler);
    HEAD(web, "/head", handler);
    OPTIONS(web, "/option", handler);

    printf("webapp running at:*8080\n");
    webapp_run(web, "*:8080", "*:8087", 0);

    webapp_destroy(web);
}