cmd_/home/toolchains/release/obj/linux-2015.11-32-mips-linux-gnu-i686-pc-linux-gnu/tmp-install/include/linux/caif/.install := /bin/sh scripts/headers_install.sh /home/toolchains/release/obj/linux-2015.11-32-mips-linux-gnu-i686-pc-linux-gnu/tmp-install/include/linux/caif ./include/uapi/linux/caif caif_socket.h if_caif.h; /bin/sh scripts/headers_install.sh /home/toolchains/release/obj/linux-2015.11-32-mips-linux-gnu-i686-pc-linux-gnu/tmp-install/include/linux/caif ./include/linux/caif ; /bin/sh scripts/headers_install.sh /home/toolchains/release/obj/linux-2015.11-32-mips-linux-gnu-i686-pc-linux-gnu/tmp-install/include/linux/caif ./include/generated/uapi/linux/caif ; for F in ; do echo "\#include <asm-generic/$$F>" > /home/toolchains/release/obj/linux-2015.11-32-mips-linux-gnu-i686-pc-linux-gnu/tmp-install/include/linux/caif/$$F; done; touch /home/toolchains/release/obj/linux-2015.11-32-mips-linux-gnu-i686-pc-linux-gnu/tmp-install/include/linux/caif/.install
