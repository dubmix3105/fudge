struct binary_format
{

    struct resource resource;
    unsigned int (*match)(struct service_protocol *protocol, struct service_backend *backend, unsigned int id);
    unsigned long (*findsymbol)(struct service_protocol *protocol, struct service_backend *backend, unsigned int id, unsigned int count, char *symbol);
    unsigned long (*findentry)(struct service_protocol *protocol, struct service_backend *backend, unsigned int id);
    unsigned long (*findbase)(struct service_protocol *protocol, struct service_backend *backend, unsigned int id, unsigned long address);
    unsigned int (*copyprogram)(struct service_protocol *protocol, struct service_backend *backend, unsigned int id);
    unsigned int (*relocate)(struct service_protocol *protocol, struct service_backend *backend, unsigned int id, unsigned int address);

};

struct binary_format *binary_findformat(struct service_protocol *protocol, struct service_backend *backend, unsigned int id);
void binary_initformat(struct binary_format *format, unsigned int (*match)(struct service_protocol *protocol, struct service_backend *backend, unsigned int id), unsigned long (*findsymbol)(struct service_protocol *protocol, struct service_backend *backend, unsigned int id, unsigned int count, char *symbol), unsigned long (*findentry)(struct service_protocol *protocol, struct service_backend *backend, unsigned int id), unsigned long (*findbase)(struct service_protocol *protocol, struct service_backend *backend, unsigned int id, unsigned long address), unsigned int (*copyprogram)(struct service_protocol *protocol, struct service_backend *backend, unsigned int id), unsigned int (*relocate)(struct service_protocol *protocol, struct service_backend *backend, unsigned int id, unsigned int address));
void binary_setupelf(void);
