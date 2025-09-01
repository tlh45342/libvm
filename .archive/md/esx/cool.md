how do I list networking information with any tool for VMWARE esxi?  What does VCENTER do to move a machine while running from one host to another?

###### ChatGPT said:

Thought for 15s

Great split question—here’s the quick, practical answer.

# How to list ESXi networking info (CLI you’ll actually use)

**Physical NICs**

```
esxcli network nic list                # vmnic*, link/speed/driver
esxcli network nic stats get -n vmnic0
```

[Support Portal](https://knowledge.broadcom.com/external/article/324496/link-state-up-or-down-for-a-vmnic-interf.html?utm_source=chatgpt.com)[Nakivo](https://www.nakivo.com/blog/most-useful-esxcli-esxi-shell-commands-vmware-environment/?utm_source=chatgpt.com)

**VMkernel (management/vMotion/vSAN/etc.)**

```
esxcli network ip interface list       # vmk* interfaces (enabled, MTU, portgroup)
esxcli network ip interface ipv4 get   # IPs on vmk*
```

[TechDocs](https://techdocs.broadcom.com/us/en/vmware-cis/vsphere/vsphere-sdks-tools/7-0/esxcli-concepts-and-examples-7-0/managing-vsphere-networking/setting-up-vsphere-networking-with-vsphere-standard-switches/adding-and-modifying-vmkernel-network-interfaces/add-and-configure-an-ipv4-vmkernel-network-interface-with-esxcli.html?utm_source=chatgpt.com)[Nakivo](https://www.nakivo.com/blog/most-useful-esxcli-esxi-shell-commands-vmware-environment/?utm_source=chatgpt.com)

**vSwitches / port groups**

```
esxcli network vswitch standard list   # vSS, uplinks, security/shaping/failover
```

(For vDS, use the vCenter UI or `net-dvs`/PowerCLI; ESXi shell shows vSS best.) [VMware Download Center](https://vdc-download.vmware.com/vmwb-repository/dcr-public/222b124e-7adf-430c-bd3d-fdf3eef99099/976ba686-c3a3-4896-b883-0b01f8acd86b/GUID-C0C8CF7A-67A8-41F4-B855-05D5014400EE.html?utm_source=chatgpt.com)

**DNS / search domains**

```
esxcli network ip dns server list
esxcli network ip dns search list
```

[Nakivo](https://www.nakivo.com/blog/most-useful-esxcli-esxi-shell-commands-vmware-environment/?utm_source=chatgpt.com)

**See which NIC/port a specific VM uses**

```
esxcli network vm list                 # note the VM’s World ID (WID)
esxcli network vm port list -w <WID>   # backing portgroup, dvport, vmnic path
```

[Support Portal](https://knowledge.broadcom.com/external/article/385143/how-to-find-which-vmnic-a-vm-is-using-on.html?utm_source=chatgpt.com)

> `vim-cmd` is great for **inventory** (VM power, datastore metadata), but for host networking state the supported tool is **`esxcli`**. [Support Portal](https://knowledge.broadcom.com/external/article/326175?utm_source=chatgpt.com)

------

# What vCenter does to live-migrate a running VM (vMotion)

At a high level, vCenter **orchestrates**, and the two ESXi hosts **transfer the VM’s state**:

1. **Prereqs checked**
   - vMotion-enabled **VMkernel NIC** on source & destination, IP reachability.
   - **CPU compatibility** (often via EVC).
   - **Storage access** (shared datastore) — or combine with **Storage vMotion** to move disks, aka **X-vMotion**.
   - Same/compatible **network port groups** so the VM keeps its MAC/IP. [VMware](https://www.vmware.com/docs/enhanced-vmotion-compatibility-evc-explained?utm_source=chatgpt.com)[VMware Blogs](https://blogs.vmware.com/vsphere/2019/07/the-vmotion-process-under-the-hood.html?utm_source=chatgpt.com)
2. **Iterative memory pre-copy**
   - Source host copies guest RAM to the destination while the VM keeps running.
   - Dirty pages are tracked and re-sent in rounds until the delta is small. [VMware Blogs](https://blogs.vmware.com/cloud-foundation/2019/07/09/the-vmotion-process-under-the-hood/?utm_source=chatgpt.com)
3. **Stun / switchover (milliseconds)**
   - Briefly pause the VM on source, copy CPU/device state + last dirty pages, then resume on the **destination**.
   - Send gratuitous ARP/ND so the network updates where the VM now lives. [VMware Blogs](https://blogs.vmware.com/vsphere/2019/07/the-vmotion-process-under-the-hood.html?utm_source=chatgpt.com)
4. **(Optional) Storage move**
   - If you also move VMDKs, Storage vMotion copies the disk while the VM runs; changed blocks are synced and cut over. (Compute+storage together = **X-vMotion**.) [VMware Blogs](https://blogs.vmware.com/vsphere/2019/07/the-vmotion-process-under-the-hood.html?utm_source=chatgpt.com)
5. **Cross-vCenter & long-distance**
   - Modern vSphere supports **Cross-vCenter vMotion** (even across different SSO domains) via the built-in Advanced XvC vMotion UI. [VMware](https://www.vmware.com/docs/vmw-introducing-the-advanced-cross-vcenter-server-vmotion-capability?utm_source=chatgpt.com)[TechDocs](https://techdocs.broadcom.com/us/en/vmware-cis/vsphere/vsphere-supervisor/7-0/requirements-for-migration-across-vcenter-servers.html?utm_source=chatgpt.com)

**Key links (good overviews):** VMware’s vMotion page, “vMotion under the hood” blog. [VMware](https://www.vmware.com/products/cloud-infrastructure/vsphere/vmotion?utm_source=chatgpt.com)[VMware Blogs](https://blogs.vmware.com/vsphere/2019/07/the-vmotion-process-under-the-hood.html?utm_source=chatgpt.com)

------

## Quick cheatsheet you can paste

```
# Host-wide view
esxcli network nic list
esxcli network ip interface list
esxcli network vswitch standard list
esxcli network ip dns server list

# VM-specific path
esxcli network vm list
esxcli network vm port list -w <WID>
```

If you tell me whether you’re on vSS or vDS (and the exact ESXi/vCenter version), I can tailor deeper commands (e.g., dvPort lookups, pktcap-uw filters, or PowerCLI one-liners).