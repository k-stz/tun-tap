#include <arpa/inet.h>
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
#include <stdbool.h>
#include <assert.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <netinet/in.h>

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

bool check_ip_in_ip_protocol(char *buffer) {
  return buffer[9] == 4 ? true : false;
}

int decapsulate_ip(char *buffer, int len) {
  memcpy(buffer, buffer+20, len-20); // in place modification?
  return len - 20;
}

int main() {
  int sock_fd, optval = 1;
  struct sockaddr_in sockaddr_in, remote;
  // SOCK_STREAM is TCP, see `man 2 socket`
  if ( (sock_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("socket()");
    exit(1);
  }
  // manipulates options at the sockets API level (as opposed to a 
  // particular protocol) 
  // level=SOL_SOCKET: 
  // optname=SO_REUSEADDR (int bool): 
  // avoid EADDRINUSE error on bind(), meaing allows to immediately rebind on 
  // the ip:port when progra restarts (or ip/portjust got free).
  // optname=&optval:
  // Supplies the value for the option SO_REUSEADDR, since the former is a boolean
  // this is should be 0 or -1.
  if(setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, (char *)&optval, sizeof(optval)) < 0) {
    perror("setsockopt()");
    exit(1);
  }
  printf("Socket created: %d", sock_fd);

  // fills struct with 0 for the length of its self...
  // effectively zero-ing out struct
  memset(&sockaddr_in, 0, sizeof(sockaddr_in));
  printf("sockaddr_in: %d\n", sockaddr_in.sin_family);

  // Define IPv4 socket address
  sockaddr_in.sin_family = AF_INET;
  const char* server_ip = "10.0.2.1";
  sockaddr_in.sin_addr.s_addr = inet_addr(server_ip);
  sockaddr_in.sin_port = htons(50000);

  printf("binding socket...\n");

  // "bind()" binds the socket to a well-known address
  if (bind(sock_fd, (struct sockaddr *) &sockaddr_in, sizeof(sockaddr_in))) {
    perror("bind()");
  }

  // listen() allows connections to be received on the socket
  if (listen(sock_fd, 5) < 0) {
    perror("listen()");
    exit(1);
  }
  printf("Server: listening on %s:%d \n", inet_ntoa(sockaddr_in.sin_addr), ntohs(sockaddr_in.sin_port));


  socklen_t remotelen = sizeof(remote);
  int client_sock_fd;
  printf("Blocking waiting for connection... \n");
  if ((client_sock_fd = accept(sock_fd, (struct sockaddr*)&remote, &remotelen)) < 0) {
    perror("accept()");
    exit(1);
  }
  printf("SERVER: Client connected from %s:%d\n", inet_ntoa(remote.sin_addr), ntohs(remote.sin_port));

  // At this point we have connected pair client_sock_fd refers to our client socket which
  // is connected to a peer socket of the client. Whose AF_INET address is stored in sockaddr remote.
  // Lets read it
  uint packet_length;
  char input_buffer[1500];
  int nread = read(client_sock_fd, (void *) input_buffer, sizeof(input_buffer));
  printf("SERVER: read %d bytes from client_socket %s:%d \n", nread, inet_ntoa(remote.sin_addr), ntohs(remote.sin_port));
  // The input buffer will contain bytes purely the payload of the TCP layer
  // the layers beneath, tcp, ip and the ethernet frame, will be handled
  // by the OS!
  // For example when nread is = 78 bytes, then in wireshark you will see, when
  // listening on the packet/segment that the payload of the TCP segment is
  // exactly 78 bytes! 
  print_buffer(input_buffer, nread, "content:");

  // Input: will thus be a ip-packet in the payload of the TCP-packet. 

  sleep(100);
}

int old_main() {
  printf("Writing Packet Example.\n");
  printf("Allocating Interface...\n");
  char device_name[] = "mytun2";
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

  // Expecting packet for decapsulation
  while(1) {
    // read returns a "ssize_t" type, this can be queried
    // with "man ssize_t" !

    // TO send data to the interface:
    // I have to ping an ip in the subnet but not the one assigned to the interface...
    printf("Ready to read on interface...\n");
    nread = read(tun_fd, buffer, sizeof(buffer)); // blocks till read
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
    write_to_file("output-server.bin", buffer, nread);
    // Now lets do something useful with the packet
    
    if (check_ip_in_ip_protocol(buffer) != true) {
      printf("No IP in IP packet received, dropping packet\n");
      continue;
    }
    int decap_packet_size = decapsulate_ip(buffer, nread);
    assert(decap_packet_size == (nread - 20));


    char ip_header[20];
    print_buffer(buffer, sizeof(ip_header), "IP of decapsulated packet");

    printf("Writing decapsulated packet to interface...\n");
    printf("IMPLEMENT ME");
    int nwrite = write(tun_fd, buffer, decap_packet_size);
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

