#define KERNEL_TASKS                    128
#define KERNEL_DESCRIPTORS              32
#define KERNEL_MOUNTS                   32
#define KERNEL_MAILBOXES                128
#define KERNEL_MAILBOXSIZE              4096

unsigned int kernel_walk(struct service_descriptor *descriptor, char *path, unsigned int length);
struct core *kernel_getcore(void);
void kernel_setcallback(struct core *(*get)(void), void (*assign)(struct task *task));
struct task *kernel_picktask(void);
struct service_mount *kernel_pickmount(void);
void kernel_usetask(struct task *task);
void kernel_usemount(struct service_mount *mount);
void kernel_freetask(struct task *task);
void kernel_freemount(struct service_mount *mount);
void kernel_assign(void);
struct service_descriptor *kernel_getdescriptor(struct task *task, unsigned int descriptor);
void kernel_copydescriptors(struct task *source, struct task *target);
void kernel_reset(unsigned int id);
unsigned int kernel_pick(unsigned int id, struct ipc_header *header, void *data);
unsigned int kernel_place(unsigned int source, unsigned int target, struct ipc_header *header, void *data);
void kernel_notify(struct list *states, unsigned int type, void *buffer, unsigned int count);
unsigned int kernel_setupbinary(struct task *task, struct service_descriptor *descriptor, unsigned int sp);
void kernel_setupinit(struct task *task);
void kernel_setup(char *buffer);
