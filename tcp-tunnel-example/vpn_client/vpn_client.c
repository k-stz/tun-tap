#include <linux/if.h>
#include <linux/if_tun.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <string.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/types.h> // ssize_t is defined here
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include <arpa/inet.h>
#include <assert.h>



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

int cwrite(int fd, char *buf, int n){
  
  int nwrite;

  if((nwrite=write(fd, buf, n)) < 0){
    perror("Writing data");
    exit(1);
  }
  return nwrite;
}

// int write_test_packet(char *buffer) {
//     char working_icmp_packet[] = { 0x45, 0x0, 0x0, 0x54, 0x64, 0xDE, 0x40, 0x0, 0x40, 
//     0x1, 0xBF, 0xA9, 0xA, 0x0, 0x0, 0x1, 0xA, 0x0, 0x2, 0x21, 0x8, 0x0, 0xE6, 0x12, 0x0, 
//     0xA7, 0x0, 0x1, 0x6, 0xB, 0x37, 0x67, 0x0, 0x0, 0x0, 0x0, 0x8, 0x0, 0xD, 0x0, 0x0, 
//     0x0, 0x0, 0x0, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 
//     0x1C, 0x1D, 0x1E, 0x1F, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 
//     0x2B, 0x2C, 0x2D, 0x2E, 0x2F, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37};
//     int size = sizeof(working_icmp_packet);
//     printf("Writing test packet with length: %d\n", size);
//     memcpy(buffer, working_icmp_packet, sizeof(working_icmp_packet));
//     return size;
// }


/* Compute checksum for count bytes starting at addr, using one's complement of one's complement sum*/
static unsigned short compute_checksum(unsigned short *addr) {
  uint count = 20;
  register unsigned int sum = 0;
  //print_buffer((char *) addr, 20, "osum buffer");
  while (count > 1) {
    //printf("osum i: %d short: %04x sum: %08x\n", count, *addr, sum);
    sum += * addr++;
    count -= 2;
  }
  //printf("osum after loop %#08x\n", sum);
  //if any bytes left, pad the bytes and add
  if(count > 0) {
    sum += ((*addr)&htons(0xFF00));
  }
  //Fold sum to 16 bits: add carrier to result
  while (sum>>16) {
      sum = (sum & 0xffff) + (sum >> 16);
  }
  //one's complement
  sum = ~sum;
  return ((unsigned short)sum);
}


u_short calc_ip_checksum(char *header) {
  // length is the last 4 bits of first byte, each representing 16bit 
  // according to IP header RFC
  int header_len = (header[0] & 0x0F) * 4;
  //printf("header_len %d\n", header_len);
  // careful, when fully implementing padding, you cant work pass-by-reference
  // we need to create a copy of the ip_header
  // Padding: ip header is muliple of 16bits, if not pad with 0xFF
  // if (header_len % 2 != 0) {
  //   printf("Padding ip_header for checksum calc\n");
  // }
  // delete checksum
  header[10] = 0x00; header[11] = 0x00;
  //print_buffer(header, header_len, "checksum cleared");
  // Sum all 2xbytes in header
  unsigned short * ip_short_header = ((unsigned short *)header);
  register uint sum = 0;
  for (int i = 0; i < header_len/2; i++) {
    u_short two_byte = ip_short_header[i];
    //printf("sum i: %d short: %04x sum: %08x\n", i, two_byte, sum);
    sum += two_byte;
  }
  printf("sum after loop %#08x\n", sum);
  // at this point sum contains carry out bits outside the 16bit frame
  // adding sum to the checksum must result in zero!
  sum = (sum >> 16) + (sum & 0xffff);
  // in case this addition created carry outs again, add them back in
  //printf("sum before adding carry outs...: %06x\n", sum);
  sum = (sum >> 16) + sum;
  //printf("sum after adding carry outs...: %06x\n", sum);
  u_short checksum = ~sum; // two's complement
  //printf("%04x(sum)  + %04x(checksum) = %04x \n", sum, checksum, sum + checksum); 
  //sum = ~sum; // <- one's complement of the sum of all shorts in order = ip checksum
  return checksum;
}



void update_ip_checksum(char *ip_header) {
  char original_header[20];
  memcpy(original_header, ip_header, 20);
  int checksum_offset = 10; // bytes 10+11 to be exact
  // this works:
  ushort * checksum_addr = (ushort *) (ip_header + checksum_offset);
  //printf("address ip_header: %p address checksum_addr %p \n", ip_header, checksum_addr);
  
  //print_buffer(ip_header, 20, "after clearing checksum");
  // ip_header[checksum_offset] = 0;
  // ip_header[checksum_offset+1] = 0;

  u_short sum = calc_ip_checksum(ip_header);
  *checksum_addr = sum;
  original_header[10] = 0;original_header[11] = 0; 
  int osum = compute_checksum((ushort *) original_header);
  if (osum != sum) {
    print_buffer(original_header, 20, "checksum calc wrong for header, using osum");
    *checksum_addr = osum;
  }
  printf("calculated ip checksum Osum: %#04x\n", osum);
  printf("calculated ip checksum mysum: %#04x\n", sum);
}

int test_ip_checksum_main() {
  // checksum is = 0xCD 0x50
  u_short correct_checksum1 = 0x50CD;
  char header[] = { 0x45, 0x00, 0x00, 0x54, 0x59, 0x56, 0x40, 0x00, 0x40, 
  0x01, 0xcd, 0x50, 0x0a, 0x00, 0x00, 0x01, 0x0a, 0x00, 0x00, 0x02 };
  // correct checksum: 0xa4f7
  u_short correct_checksum2 = 0xa4f7; 
  char header2[] = { 0x45, 0x00, 0x00, 0x68, 0x2c, 0xcc, 0x40, 0x00, 0x40, 0x04, 0x00, 
  0x00, 0x0a, 0x00, 0x00, 0x01, 0x0a, 0x00, 0x02, 0x21 };

  char header3[] = { 0x45, 0x00, 0x00, 0x68, 0x2c, 0xcc, 0x40, 0x00, 0x40, 0x04, 0x00, 
  0x00, 0x0a, 0x00, 0x00, 0x01, 0x0a, 0x00, 0x02, 0x21 };
  header3[10] = 0; header3[11] = 0;

  u_short checksum;
  header[10] = 0; header[11] = 0;
  checksum = calc_ip_checksum(header);
  printf("test1: 0x%04X == 0x%04X %s\n", checksum, correct_checksum1, (checksum == correct_checksum1) ? "true" : "false");


  header2[10] = 0; header2[11] = 0;
  checksum = calc_ip_checksum(header2);
  printf("test2: 0x%04X == 0x%04X %s\n", checksum, correct_checksum2, (checksum == correct_checksum2) ? "true" : "false");
  checksum = compute_checksum((u_short *) header3);
  printf("test3: 0x%04X == 0x%04X %s\n", checksum, correct_checksum2, (checksum == correct_checksum2) ? "true" : "false");



  u_short correct_checksum = 0x6725;
  //char ip_header[] = {0x45, 0x00, 0x00, 0x68, 0xbd, 0x4b, 0x40, 0x00, 0x40, 0x04, 
  //0x67, 0x26, 0x0a, 0x00, 0x00, 0x01, 0x0a, 0x00, 0x02, 0x21};
  //int sum = calc_ip_checksum(ip_header);
  //printf("final sum: %#06x\n", sum);
  //printf("final osum %#06x\n:", compute_checksum((unsigned short *) ip_header));
  return 0;
}

// Returns client socket connecting to vpn
int connect_vpn(const char *ip_address, int port) {
  struct sockaddr_in remote;
  remote.sin_family = AF_INET;
  remote.sin_addr.s_addr = inet_addr(ip_address);
  remote.sin_port = htons(port);
  int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
  int result = connect(socket_fd, (const struct sockaddr *) &remote, sizeof(remote));
  if (result < 0) {
    perror("connect()");
    printf("Is the target vpn_server running?\n");
    exit(1);
  }

  return socket_fd;
}

int test_connection() {
  // test writing to vpn_server.c
  const char* vpn_server_ip = "10.0.2.1";
  int vpn_port = 50000;
  int socket_fd = connect_vpn("10.0.2.1", vpn_port);
  printf("Client: connected to server %s:%d\n", vpn_server_ip, vpn_port);
  printf("Client: writing some data...\n");
  int nwrite = write(socket_fd, "testing\n", 7);
  printf("Client: written %d bytes to sock stream\n", nwrite);
  return 0;
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
  printf("%s allocated successfully\n", device_name);

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
    write_to_file("output-client.bin", buffer, nread);
    // Now lets do something useful with the packet
    
    // buffer[17] = 1; mytap as target doesn't work, not even
    // from cli, so imply sending packet with other destination 
    char encapsulating_ip_header[20];
    memcpy(encapsulating_ip_header, buffer, 20);
    print_buffer(buffer, sizeof(encapsulating_ip_header), "after writing encapsulating_ip_header");

    char dst_ip[] = {10,0,2,1}; // 10.0.2.0/24 is subnet of vpn-server!
    memcpy(encapsulating_ip_header+16, dst_ip, 4);
    // encapsulating_ip_header[9] = 4;
    
    char src_ip[] = {201,2,2,2}; // so no drop due to martian packet

    memcpy(encapsulating_ip_header+12, src_ip, 4);

    // set protocol for outer-most encapsulating ip-packet to 4, 
    // which indicates "IP in IP" according to RFC 2003
    

    char original_packet[nread];
    memcpy(original_packet,buffer, nread);
    print_buffer(original_packet, sizeof(original_packet), "Original IP Packet (will be encapsulated unchanged)");
    int tunneled_packet_size = sizeof(encapsulating_ip_header) + sizeof(original_packet);

    //  add total size of tunneled packet to ip header
    encapsulating_ip_header[3] = tunneled_packet_size; // TODO size might be wrong

    // Recalculate IP Header Checksum
    // Very important: now that we're done tampering with the IP haeder,
    // we need to calcluate its error-checking checksum header, else the
    // packet will be silently dropped by the kernel routing!
    update_ip_checksum(encapsulating_ip_header);

    print_buffer(encapsulating_ip_header, sizeof(encapsulating_ip_header), "encapsulating_ip_header (final version)");


    /* Piece together final packet in buffer */
    // 1. Write encapsulating_ip_header
    memcpy(buffer, encapsulating_ip_header, sizeof(encapsulating_ip_header));
    // 2. After it (+sizeof...) write orignal packet in buffer
    memcpy(buffer+sizeof(encapsulating_ip_header), original_packet, sizeof(original_packet));
    
    //print_buffer(buffer, tunneled_packet_size, "\nBuffer with tunneled packet");


    //printf("Writing to interface...\n");
    const char* vpn_server_ip = "10.0.2.1";
    int vpn_port = 50000;

    int vpn_client_socket = connect_vpn(vpn_server_ip, vpn_port);
    printf("Client: connected to server %s:%d\n", vpn_server_ip, vpn_port);

    // Now we write the packet into the socket 
    //int nwrite = cwrite(vpn_client_socket, buffer, tunneled_packet_size);
      //plength = htons(nread);
      //nwrite = cwrite(net_fd, (char *)&plength, sizeof(plength));
      //nwrite = cwrite(net_fd, buffer, nread);
    uint16_t plength = htons(tunneled_packet_size);
    int nwrite;
    cwrite(vpn_client_socket, (char *)&plength, sizeof(plength));
    printf("CLIENT: plength = %d written to socket \n", ntohs(plength));
    nwrite = cwrite(vpn_client_socket, buffer, tunneled_packet_size);
     //(vpn_client_socket, message, 7);

    if (nwrite < 0) {
      perror("writing to interface\n");
    }
    printf("CLIENT: tunneled packet written, size: %d bytes!\n", nwrite);
    sleep(1000);  
    // // doesn't seem to cause an endless loop... sleep not needed
    // //
    // printf("sleep 10 after writing packet! (could be endless loop!)\n");
    sleep(10);
    exit(0);
  }
}

