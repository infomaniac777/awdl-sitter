#include <stdio.h>
#include <string.h>
#include <syslog.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <unistd.h>
#include <SystemConfiguration/SystemConfiguration.h>

static void bring_awdl_down(void) {
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) return;

    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));
    strlcpy(ifr.ifr_name, "awdl0", sizeof(ifr.ifr_name));

    if (ioctl(sock, SIOCGIFFLAGS, &ifr) == 0 && (ifr.ifr_flags & IFF_UP)) {
        ifr.ifr_flags &= ~IFF_UP;
        if (ioctl(sock, SIOCSIFFLAGS, &ifr) == 0)
            syslog(LOG_NOTICE, "brought awdl0 down");
    }

    close(sock);
}

static void sc_callback(SCDynamicStoreRef store, CFArrayRef changedKeys, void *info) {
    bring_awdl_down();
}

int main(void) {
    openlog("awdl-killer", LOG_PID, LOG_DAEMON);

    bring_awdl_down();

    SCDynamicStoreContext ctx = { 0, NULL, NULL, NULL, NULL };
    SCDynamicStoreRef store = SCDynamicStoreCreate(NULL, CFSTR("awdl-killer"), sc_callback, &ctx);
    if (!store) {
        syslog(LOG_ERR, "failed to create SCDynamicStore");
        return 1;
    }

    // State:/Network/Interface/awdl0/Link fires the moment the kernel raises awdl0
    CFStringRef key = SCDynamicStoreKeyCreateNetworkInterfaceEntity(
        NULL,
        kSCDynamicStoreDomainState,
        CFSTR("awdl0"),
        kSCEntNetLink
    );
    CFArrayRef keys = CFArrayCreate(NULL, (const void **)&key, 1, &kCFTypeArrayCallBacks);
    SCDynamicStoreSetNotificationKeys(store, keys, NULL);
    CFRelease(key);
    CFRelease(keys);

    CFRunLoopSourceRef source = SCDynamicStoreCreateRunLoopSource(NULL, store, 0);
    CFRunLoopAddSource(CFRunLoopGetMain(), source, kCFRunLoopDefaultMode);
    CFRelease(source);

    syslog(LOG_NOTICE, "watching awdl0");
    CFRunLoopRun();

    CFRelease(store);
    closelog();
    return 0;
}
