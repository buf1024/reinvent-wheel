#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef struct webapp_s webapp_t;
typedef struct webapp_context_s webapp_context_t;
typedef void (*webapp_handler_t)(webapp_context_t*);

typedef struct webapp_url_s webapp_url_t;
typedef struct webapp_request_s webapp_request_t;
typedef struct webapp_response_s webapp_response_t;

#define GET(w, p, h)     webapp_get((w), (p), (h), 0)
#define POST(w, p, h)    webapp_post((w), (p), (h), 0) 
#define PUT(w, p, h)     webapp_put((w), (p), (h), 0) 
#define DELETE(w, p, h)  webapp_delete((w), (p), (h), 0) 
#define HEAD(w, p, h)    webapp_head((w), (p), (h), 0) 
#define OPTIONS(w, p, h) webapp_options((w), (p), (h), 0) 

struct webapp_url_s {
	//Scheme   string
	//Opaque   string    // encoded opaque data
	//User     *Userinfo // username and password information
	char* host;    // host or host:port
	char* path;
	//RawPath  string // encoded path hint (Go 1.5 and later only; see EscapedPath method)
	//RawQuery string // encoded query values, without '?'
	//Fragment string // fragment for references, without '#'
};

struct webapp_request_s {
    char* method;
    webapp_url_t* url;
    void* headers; 

};

struct webapp_response_s {

};

struct webapp_context_s {
    webapp_request_t* req;
    webapp_response_t* rsp;
};


webapp_t* webapp_new();
void webapp_destroy(webapp_t* web);

int webapp_run(webapp_t* web, const char* host, ...);

webapp_t* webapp_use(webapp_t* web, webapp_handler_t handler);

webapp_t* webapp_group_begin(webapp_t* web, const char* pattern);
webapp_t* webapp_group_end(webapp_t* web);

webapp_t* webapp_get(webapp_t* web, const char* pattern, ...);
webapp_t* webapp_post(webapp_t* web, const char* pattern, ...);
webapp_t* webapp_put(webapp_t* web, const char* pattern, ...);
webapp_t* webapp_delete(webapp_t* web, const char* pattern, ...);
webapp_t* webapp_head(webapp_t* web, const char* pattern, ...);
webapp_t* webapp_options(webapp_t* web, const char* pattern, ...);
