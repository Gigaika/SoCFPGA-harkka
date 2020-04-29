APP = classifier

# Add any other object files to this list below
APP_OBJS = classifier.o devices.o interface.o

all: build

build: $(APP)

$(APP): $(APP_OBJS)
	$(CC) -o $@ $(APP_OBJS) $(LDFLAGS) $(LDLIBS) -lpthread
clean:
	rm -f $(APP) *.o
