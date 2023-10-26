Role Name
=========

This role sets up a Wireguard server and its configuration file. The Wireguard configuration is compatible with 2 peers.

Role Variables
--------------

wireguard_server_address: the address the server uses to be accessible through the Wireguard tunnel.
allowed_ips1: the range of IPs that are allowed through the tunnel from the first peer.
allowed_ips2: the range of IPs that are allowed through the tunnel from the second peer.
listen_port: the port the server listens on.

Example Playbook
----------------
  vars:
    wireguard_server_address: "10.50.50.1/24"
    allowed_ips1: "10.50.50.2"
    allowed_ips2: "10.50.50.3"
    listen_port: 2709
  roles:
     - role: wireguard-setup

Author Information
------------------

Made by Van Loon Bernd (GitHub: BerrieV1)
