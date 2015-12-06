#include "broadcast.h"

// useful doc at http://man7.org/linux/man-pages/man3/getaddrinfo.3.html

// ping packets consist solely of their TCP server's port number,
// which is a uint16, followed by PING_CODE, which is
// a uint32, so we're never going to be interested in any
// message longer than 2 + 4 = 6 bytes
#define PACKET_LEN 6

// a port can never be longer than 5 chars, +1 to NULL terminate
#define PORT_STRING_LEN 6

// FIXME: should we reset the socket every now and then?
// in particular test with the computer going to sleep mode, and such
void broadcast_listen(logger_t* logger)
{
  int sock_fd, socket_fds[FD_SETSIZE], socket_count = 0, highest_sock_fd = -1;
  int i, error_code, bytes_received_count;
  fd_set socket_fds_set;
  char buffer[PACKET_LEN + 1], port_string[PORT_STRING_LEN], host_info[NI_MAXHOST + NI_MAXSERV + 2], host[NI_MAXHOST], service[NI_MAXSERV];
  struct addrinfo hints, *address_list_head, *listen_address;
  struct sockaddr_storage their_address;
  socklen_t my_addr_len, their_address_len;
  uint16_t their_tcp_port;
  uint32_t ping_code_verif;
  size_t host_len;

  memset(&hints, 0, sizeof(struct addrinfo));
  // FIXME: actually test with ipv6
  hints.ai_family = AF_UNSPEC; // ipv4 or 6
  hints.ai_socktype = SOCK_DGRAM; // datagram socket
  hints.ai_flags = AI_PASSIVE; // wildcard IP address

  snprintf(port_string, 6, "%d", BROADCAST_PORT);
  error_code = getaddrinfo(NULL, port_string, &hints, &address_list_head);
  if (error_code != 0) {
    fatal(logger, "listen: getaddrinfo: %s\n", gai_strerror(error_code));
    exit(1);
  }

  // loop through all the results and bind to all we can
  for(listen_address = address_list_head; listen_address != NULL; listen_address = listen_address->ai_next) {
    my_addr_len = sizeof(struct addrinfo);
    error_code = getnameinfo(listen_address->ai_addr, my_addr_len,
                             host, NI_MAXHOST,
                             service, NI_MAXSERV, NI_NUMERICSERV);
    if (error_code == 0) {
      host_len = strlen(host);
      memcpy(host_info, host, host_len);
      memcpy(host_info + host_len, "/", 1);
      memcpy(host_info + host_len + 1, service, strlen(service) + 1);
    } else {
      warning(logger, "listen: failed to get human-readable host info");
      host_info[0] = '\0';
    }

    sock_fd = socket(listen_address->ai_family, listen_address->ai_socktype, listen_address->ai_protocol);
    
    if (sock_fd == -1) {
      warning(logger, "listen: failed to create socket fd for %s", host_info);
      continue;
    }

    error_code = bind(sock_fd, listen_address->ai_addr, listen_address->ai_addrlen);
    if (error_code != 0) {
      warning(logger, "listen: failed to bind to %s: %d", host_info, error_code);
      close(sock_fd);
      continue;
    }

    // success!
    info(logger, "listen: successfully bound to %s", host_info);
    // add the socket FD to the set of FDs we'll watch
    socket_fds[socket_count] = sock_fd;
    socket_count++;
    if (sock_fd > highest_sock_fd) {
      highest_sock_fd = sock_fd;
    }
  }

  // no longer needed
  freeaddrinfo(address_list_head);

  if (socket_count == 0) {
    fatal(logger, "listen: failed to bind any socket");
    exit(1);
  }

  // now we can listen for incoming pings
  their_address_len = sizeof(struct sockaddr_storage);
  while(1) {
    // select modifies the fd_set in place, so we need to rebuild it every time
    FD_ZERO(&socket_fds_set);
    for (i = 0; i < socket_count; i++) {
      FD_SET(socket_fds[i], &socket_fds_set);
    }

    // then wait for someone to ping us!
    error_code = select(highest_sock_fd + 1, &socket_fds_set, NULL, NULL, NULL);
    if (error_code == -1) {
      warning(logger, "listen: error while waiting for a ping: %s", strerror(errno));
    } else {
      debug(logger, "listen: select returned with %d", error_code);

      for (i = 0; i < socket_count; i++) {
        sock_fd = socket_fds[i];
        if (FD_ISSET(sock_fd, &socket_fds_set)) {
          bytes_received_count = recvfrom(sock_fd, buffer, PACKET_LEN, 0,
                                          (struct sockaddr*) &their_address, &their_address_len);

          switch(bytes_received_count) {
            case PACKET_LEN:
              // TODO wkpo get the IP
              memcpy(&their_tcp_port, buffer, 2);
              memcpy(&ping_code_verif, buffer + 2, 4);
              info(logger, "listen: received a ping from TODO wkpo with tcp_port: %d and code verif %d", their_tcp_port, ping_code_verif);
              break;
            case -1:
              warning(logger, "listen: ignoring failed broadcast request");
              break;
            default:
              debug(logger, "listen: received an unexpected broadcast packet of length %d", bytes_received_count);
          }
        }
      }
    }
  }
}

// broadcasts the TCP port on the subnet
// inspired from https://gist.github.com/neilalexander/3020008
int broadcast_emit(logger_t* logger, uint16_t tcp_port)
{
  struct ifaddrs *interface_list_head = NULL, *interface;
  int error_code, sock_fd = -1, return_value = 0, sock_optval;
  char buffer[PACKET_LEN], *iface_name;
  uint32_t ping_code_verif = PING_CODE;
  ssize_t bytes_sent_count;
  struct sockaddr_in send_addr, *iface_addr;
  socklen_t send_addr_len;

  // create a raw socket
  sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sock_fd == -1) {
    fatal(logger, "emit: failed to create socket fd: %s", strerror(errno));
    goto fail;
  }

  // set the right sock options to send broadcast packets
  error_code = setsockopt(sock_fd, SOL_SOCKET, SO_BROADCAST, &sock_optval,
                          sizeof(int));
  if (error_code == -1) {
    fatal(logger, "emit: setsockopt (SO_BROADCAST): %s", strerror(errno));
    goto fail;
  }

  // get a list of all interfaces
  error_code = getifaddrs(&interface_list_head);
  if (error_code != 0) {
    fatal(logger, "emit: could not list interfaces: %s", strerror(errno));
    goto fail;
  }

  // prepare the packet to be sent
  memcpy(buffer, &tcp_port, 2);
  memcpy(buffer + 2, &ping_code_verif, 4);

  // set address options
  memset(&send_addr, 0, sizeof(struct sockaddr_in));
  send_addr.sin_family = AF_INET;
  send_addr.sin_port = htons(BROADCAST_PORT);
  send_addr_len = sizeof(struct sockaddr_in);

  for(interface = interface_list_head; interface != NULL; interface = interface->ifa_next) {
    iface_name = interface->ifa_name;
    iface_addr = NULL;

#ifdef __linux__
    if (interface->ifa_flags & IFF_BROADCAST) {
      iface_addr = (struct sockaddr_in*) interface->ifa_broadaddr;
    } else if (interface->ifa_flags & IFF_POINTOPOINT) {
      iface_addr = (struct sockaddr_in*) interface->ifa_dstaddr;
    }
#else
    iface_addr = (struct sockaddr_in*) interface->ifa_dstaddr;
#endif

    if (iface_addr == NULL) {
      debug(logger, "emit: iface %s does not define any iface address, skipping", iface_name);
      continue;
    }

    if (iface_addr->sin_family != AF_INET) {
      // FIXME: what about IPv6??
      debug(logger, "emit: iface %s's address not an AF_INET, skipping", iface_name);
      continue;
    }

    if (iface_addr->sin_addr.s_addr == 0) {
      debug(logger, "emit: iface %s's address empty, skipping", iface_name);
      continue;
    }

    // set the address
    send_addr.sin_addr = iface_addr->sin_addr;

    // send the packet!
    bytes_sent_count = sendto(sock_fd, &buffer, PACKET_LEN, 0,
                              (struct sockaddr*) &send_addr, send_addr_len);

    if (bytes_sent_count == PACKET_LEN) {
      debug(logger, "emit: successfully sent broadcast packet on iface %s with TCP port %"PRIu16,
            iface_name, tcp_port);
    } else if (bytes_sent_count == -1) {
      error(logger, "emit: unable to send for iface %s: %s",
            iface_name, strerror(errno));
    } else {
      // partially sent
      error(logger, "emit: unable to send full packet for iface %s, sent %d bytes",
            iface_name, bytes_sent_count);
    }
  }

  goto cleanup;

fail:
  return_value = 1;

cleanup:
  if (sock_fd != -1) {
    close(sock_fd);
  }
  if (interface_list_head != NULL) {
    freeifaddrs(interface_list_head);
  }
  return return_value;
}
