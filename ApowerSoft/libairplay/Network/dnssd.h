#ifndef DNSSD_H
#define DNSSD_H
#include <vector>
#include<string>
#if defined(_WIN32) && defined(DLL_EXPORT)
# define DNSSD_API //__declspec(dllexport)
#else
# define DNSSD_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define DNSSD_ERROR_NOERROR       0
#define DNSSD_ERROR_HWADDRLEN     1
#define DNSSD_ERROR_OUTOFMEM      2
#define DNSSD_ERROR_LIBNOTFOUND   3
#define DNSSD_ERROR_PROCNOTFOUND  4

typedef struct dnssd_s dnssd_t;
DNSSD_API dnssd_t *dnssd_init(int *error);
DNSSD_API int dnssd_register(dnssd_t * dnssd, const char * servname, const char * tcpname, unsigned short port,std::vector<std::pair<std::string, std::string> > txt);
DNSSD_API void dnssd_unregister(dnssd_t *dnssd);
DNSSD_API void dnssd_destroy(dnssd_t *dnssd);
DNSSD_API void dnssd_update(dnssd_t* dnssd, bool boddnum);

#ifdef __cplusplus
}
#endif
#endif
