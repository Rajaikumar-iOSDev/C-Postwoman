#include "network.h"
#include <curl/curl.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

static struct HttpResponse g_response = { NULL, 0 };
static pthread_mutex_t g_resp_mutex = PTHREAD_MUTEX_INITIALIZER;
static bool g_loading = false;

// Append callback for libcurl
static size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    struct HttpResponse *resp = (struct HttpResponse *)userp;
    char *ptr = realloc(resp->data, resp->size + realsize + 1);
    if (!ptr) return 0; // out of memory
    resp->data = ptr;
    memcpy(&(resp->data[resp->size]), contents, realsize);
    resp->size += realsize;
    resp->data[resp->size] = 0;
    return realsize;
}

// Thread fetch arguments
struct FetchArgs {
    char url[2048];
    char *body;  // Optional POST body
};

static void *fetch_thread(void *vargs) {
    struct FetchArgs *args = (struct FetchArgs *)vargs;
    struct HttpResponse chunk = { NULL, 0 };

    CURL *curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, args->url);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 15L);

        // If body is provided, set up POST request
        if (args->body) {
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, args->body);
            curl_easy_setopt(curl, CURLOPT_POST, 1L);
        }

        CURLcode res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);
    }

    // Move chunk into global response under mutex
    pthread_mutex_lock(&g_resp_mutex);
    if (g_response.data) free(g_response.data);
    g_response = chunk;
    pthread_mutex_unlock(&g_resp_mutex);

    g_loading = false;
    if (args->body) free(args->body);
    free(args);
    return NULL;
}

void network_init(void) {
    curl_global_init(CURL_GLOBAL_ALL);
}

void network_cleanup(void) {
    pthread_mutex_lock(&g_resp_mutex);
    if (g_response.data) {
        free(g_response.data);
        g_response.data = NULL;
        g_response.size = 0;
    }
    pthread_mutex_unlock(&g_resp_mutex);
    curl_global_cleanup();
}

bool network_fetch_async(const char *url, const char *body) {
    if (g_loading) return false;
    
    struct FetchArgs *args = malloc(sizeof(struct FetchArgs));
    if (!args) return false;
    
    args->url[0] = 0;
    strncpy(args->url, url, sizeof(args->url)-1);
    
    if (body) {
        args->body = strdup(body);
        if (!args->body) {
            free(args);
            return false;
        }
    } else {
        args->body = NULL;
    }
    
    g_loading = true;
    
    // Clear previous response
    pthread_mutex_lock(&g_resp_mutex);
    if (g_response.data) {
        free(g_response.data);
        g_response.data = NULL;
        g_response.size = 0;
    }
    pthread_mutex_unlock(&g_resp_mutex);
    
    pthread_t tid;
    if (pthread_create(&tid, NULL, fetch_thread, args) != 0) {
        if (args->body) free(args->body);
        free(args);
        g_loading = false;
        return false;
    }
    pthread_detach(tid);
    return true;
}

bool network_is_loading(void) {
    return g_loading;
}

const struct HttpResponse *network_get_response(void) {
    return &g_response;
}