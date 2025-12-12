#ifndef NETWORK_H
#define NETWORK_H

#include <stdbool.h>
#include <stddef.h>

// Response data structure
struct HttpResponse {
    char *data;
    size_t size;
};

// Initialize networking subsystem
void network_init(void);

// Cleanup networking subsystem
void network_cleanup(void);

// Start an async HTTP request
// Returns true if request was started successfully
bool network_fetch_async(const char *url, const char *body);

// Check if a request is currently in progress
bool network_is_loading(void);

// Get the current response (returns NULL if no response available)
// The returned pointer remains valid until the next fetch
const struct HttpResponse *network_get_response(void);

#endif // NETWORK_H