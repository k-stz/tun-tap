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

// void print_buffer(char *buffer, size_t size, int use_hex) {
//     for (size_t i = 0; i < size; i += 16) {
//         printf("%08lx  ", i);  // Print offset in hex
        
//         // Print each byte in hex format with spaces in between
//         for (size_t j = 0; j < 16; ++j) {
//             if (i + j < size) {
//                 printf("%02X ", buffer[i + j]);
//             } else {
//                 printf("   ");  // Fill if line is shorter than 16 bytes
//             }

//             if (j == 7) printf(" ");  // Extra space between 8th and 9th byte
//         }

//         // If hex mode is disabled, print ASCII representation
//         if (use_hex) {
//             printf(" |");
//             for (size_t j = 0; j < 16; ++j) {
//                 if (i + j < size) {
//                     unsigned char ch = buffer[i + j];
//                     printf("%c", isprint(ch) ? ch : '.');  // Print ASCII or '.' for non-printables
//                 } else {
//                     printf(" ");  // Fill if line is shorter than 16 bytes
//                 }
//             }
//             printf("|");
//         }

//         printf("\n");
//     }
// }

void print_buffer(char *buffer, int size, char *print_message ) {
  printf(print_message);
  printf(":\n");
  for (int i=0; i < size; i++ ) {
    printf("%02x ", (unsigned char)buffer[i]);
  }
  printf("\n\n");
}

void write_to_file(char *filename, char *data, size_t size) {
  FILE *file = fopen(filename, "w");
  if (file == NULL) {
      perror("Failed to open file");
      exit(1);
  }

  size_t bytes_written = fwrite(data, sizeof(char), size, file);
    if (bytes_written < size) {
        perror("Failed to write all data to file");
        fclose(file);
        exit(1);
    }
  fclose(file);
}

int main() {
  printf("Writing Packet Example.\n");
  printf("Allocating Interface...\n");
  char device_name[] = "mytun";
  int tun_fd;
  // this will allocate the tun device
  // IFF_NO_PI will make sure we receive "pure packets" with no leading
  // packet information bytes (proto)
  tun_fd = tun_alloc(device_name, IFF_TUN | IFF_NO_PI);
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
    // print_buffer(buffer, 50, 1);
    for (size_t i = 0; i < nread; i++) {
      printf("%01X ", (unsigned char)buffer[i]);
    }
    printf("\n");
    write_to_file("output-file.bin", buffer, nread);
    // Now lets do something useful with the packet
    
    // buffer[17] = 1; mytap as target doesn't work, not even
    // from cli, so imply sending packet with other destination 
    char ip_header[20];
    memcpy(ip_header, buffer, 20);
    print_buffer(buffer, sizeof(ip_header), "after writing ip_header");

    char dst_ip[] = {10,0,0,42};
    memcpy(ip_header+16, dst_ip, 4);
    print_buffer(ip_header, sizeof(ip_header), "ip_header new dst_ip");

    // set protocol for outer-most ip header to 4, which indicates "IP in IP" according to RFC 2003

    ip_header[9] = 4;

    char whole_packet[nread];
    memcpy(whole_packet,buffer, nread);
    print_buffer(whole_packet, sizeof(whole_packet), "whole packet after buffer memcpy into it");

    memcpy(buffer+sizeof(ip_header), whole_packet, sizeof(whole_packet));
    int tunneled_packet_size = sizeof(ip_header) + sizeof(whole_packet);

    // NOw lets add total size of tunneled packet to ip header
    ip_header[3] = tunneled_packet_size;
    // now lets join ip_header+whole_packet to tunnel the "whole_packet" inside it

    memcpy(buffer, ip_header, sizeof(ip_header));


    printf("Tunneled packet:\n");
    print_buffer(buffer, tunneled_packet_size, "Buffer with tunneled packet");

    printf("Writing to interface...\n");
    int nwrite = write(tun_fd, buffer, tunneled_packet_size);
    if (nwrite < 0) {
      perror("writing to interface\n");
    }
    printf("tunneled packet written!\n");
    // doesn't seem to cause an endless loop... sleep not needed
    //
    printf("sleep 10 after writing packet! (could be endless loop!)\n");
    sleep(10);
  }
}

