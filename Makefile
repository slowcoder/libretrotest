CC := gcc
CFLAGS := -Wall -std=gnu99 -g
LDFLAGS := -g -ldl -lSDL -lz

OBJS := main.o retrocore.o retrocore_env.o

ALL_DEPS := $(patsubst %.o,%.d,$(OBJS))

all: lrtest

clean:
	rm -f $(OBJS) $(ALL_DEPS) lrtest

lrtest: $(OBJS)
	@echo "LD $@"
	@$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o:%.c
	@echo "CC $<"
	@$(CC) -MM -MF $(subst .o,.d,$@) -MP -MT "$@ $(subst .o,.d,$@)" $(CFLAGS) $<
	@$(CC) $(CFLAGS) -c -o $@ $<

ifneq ("$(MAKECMDGOALS)","clean")
-include $(ALL_DEPS)
endif
