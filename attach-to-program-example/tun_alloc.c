#include <linux/if.h>
#include <linux/if_tun.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h> // ssize_t is defined here
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>


int tun_alloc(char *dev, int flags) {

  struct ifreq ifr;
  int fd, err;
  char *clonedev = "/dev/net/tun";

  /* Arguments taken by the function:
   *
   * char *dev: the name of an interface (or '\0'). MUST have enough
   *   space to hold the interface name if '\0' is passed
   * int flags: interface flags (eg, IFF_TUN etc.)
   */

   /* open the clone device */
   if( (fd = open(clonedev, O_RDWR)) < 0 ) {
     return fd;
   }

   /* preparation of the struct ifr, of type "struct ifreq" */
   memset(&ifr, 0, sizeof(ifr));

   ifr.ifr_flags = flags;   /* IFF_TUN or IFF_TAP, plus maybe IFF_NO_PI */

   if (*dev) {
     /* if a device name was specified, put it in the structure; otherwise,
      * the kernel will try to allocate the "next" device of the
      * specified type */
     strncpy(ifr.ifr_name, dev, IFNAMSIZ);
   }

   /* try to create the device */
   if( (err = ioctl(fd, TUNSETIFF, (void *) &ifr)) < 0 ) {
     close(fd);
     return err;
   }

  /* if the operation was successful, write back the name of the
   * interface to the variable "dev", so the caller can know
   * it. Note that the caller MUST reserve space in *dev (see calling
   * code below) */
  strcpy(dev, ifr.ifr_name);

  /* this is the special file descriptor that the caller will use to talk
   * with the virtual interface */
  return fd;
}

void print_buffer(char *buffer, size_t size, int use_hex) {
    for (size_t i = 0; i < size; i += 16) {
        printf("%08lx  ", i);  // Print offset in hex
        
        // Print each byte in hex format with spaces in between
        for (size_t j = 0; j < 16; ++j) {
            if (i + j < size) {
                printf("%02X ", buffer[i + j]);
            } else {
                printf("   ");  // Fill if line is shorter than 16 bytes
            }

            if (j == 7) printf(" ");  // Extra space between 8th and 9th byte
        }

        // If hex mode is disabled, print ASCII representation
        if (use_hex) {
            printf(" |");
            for (size_t j = 0; j < 16; ++j) {
                if (i + j < size) {
                    unsigned char ch = buffer[i + j];
                    printf("%c", isprint(ch) ? ch : '.');  // Print ASCII or '.' for non-printables
                } else {
                    printf(" ");  // Fill if line is shorter than 16 bytes
                }
            }
            printf("|");
        }

        printf("\n");
    }
}

int main() {
  printf("Allocating Interface...\n");
  char device_name[] = "mytun";
  //strcpy(device_name, "mytun");
  int tun_fd;
  // this will allocate the tun device
  tun_fd = tun_alloc(device_name, IFF_TUN);
  if(tun_fd < 0) {
    perror("Allocating interface");
    exit(1);
  }
  printf("%s allocated successfullyy\n", device_name);

  // I guess now we're attached to the device

  // buffer must be at least the size of the MTU!
  // By deafult, for ethernet, that's 1500. But from tests 
  // it accepts any larger size, probably up to 64k (IP limit)
  char buffer[0x1000]; // hex, thus 4096 bytes
  ssize_t nread;
  // read data coming from the kernel
  while(1) {
    // read returns a "ssize_t" type, this can be queried
    // with "man ssize_t" !

    // TO send data to the interface:
    // I have to ping an ip in the subnet but not the one assigned to the interface...
    printf("Ready to read on interface...\n");
    nread = read(tun_fd, buffer, sizeof(buffer));
    if(nread < 0) {
      perror("reading from interface");
      close(tun_fd);
      exit(1);
    }

    // finally do something with data on interface!
    printf("# Read %zd bytes from device %s\n", nread, device_name);
    print_buffer(buffer, 50, 1);
    for (size_t i = 0; i < nread; i++) {
      printf("%c ", buffer[i]);
    }
  }
}

