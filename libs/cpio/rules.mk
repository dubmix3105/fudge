LIBCPIO:=$(LIBS_PATH)/cpio/libcpio.a
LIBCPIO_OBJ:=$(LIBS_PATH)/cpio/cpio.o

$(LIBCPIO): $(LIBCPIO_OBJ)
	$(AR) $(ARFLAGS) $@ $^

LIBS+=$(LIBCPIO)
LIBS_OBJECTS+=$(LIBCPIO_OBJ)