how does vim-cmd for esxi view or configure DATASTORES

###### ChatGPT said:

Thought for 21s

Great question. On **ESXi**, `vim-cmd` can *inspect* and do a few light ops on datastores, but most **configuration** (add/remove/mount) is done with `esxcli`.

## What `vim-cmd` can do with datastores

List all datastores (name, type, capacity):

```
vim-cmd hostsvc/datastore/listsummary
```

[communities.vmware.com](https://communities.vmware.com/t5/ESXi-Discussions/Vmware-and-datastores/td-p/2560244?utm_source=chatgpt.com)[jasonpenniman.com](https://www.jasonpenniman.com/identifying-datastores-configured-in-vmware-server-20?utm_source=chatgpt.com)

Get detailed info for one datastore (handy for “forceMounted” checks):

```
vim-cmd hostsvc/datastore/info <datastore-name>
```

[Support Portal](https://knowledge.broadcom.com/external/article/323084/identify-if-a-datastore-is-force-mounted.html?utm_source=chatgpt.com)

Rename a datastore:

```
vim-cmd hostsvc/datastore/rename <oldName> <newName>
```

[NetworkProGuide](https://networkproguide.com/wp-content/uploads/vmware-esxi-command-line-cheat-sheet-v1-1-printable.pdf?utm_source=chatgpt.com)[Gist](https://gist.github.com/sagunms/96a39e2064c8837fbd71?utm_source=chatgpt.com)

Refresh a datastore’s view (kick the host to re-check it):

```
vim-cmd hostsvc/datastore/refresh <datastore-name>
```

[ManageEngine | Community and Support](https://pitstop.manageengine.com/portal/en/community/topic/vmware-monitoring-esxi-disk-free-space-utalization?utm_source=chatgpt.com)

> Tip: For general filesystem visibility you can also check `/vmfs/volumes` and use `esxcli storage filesystem list`. [Server Fault](https://serverfault.com/questions/569122/where-the-vm-files-are-stored-on-vmware-esxi?utm_source=chatgpt.com)[Virtualization DOJO](https://virtualizationdojo.com/vmware/top-20-esxcli-commands/?utm_source=chatgpt.com)

## For (re)configuring storage, use `esxcli`

`vim-cmd` doesn’t add/mount NFS/VMFS. Use `esxcli`:

List filesystems (shows datastore names, UUIDs, paths):

```
esxcli storage filesystem list
```

[Virtualization DOJO](https://virtualizationdojo.com/vmware/top-20-esxcli-commands/?utm_source=chatgpt.com)

Add an **NFS v3** datastore:

```
esxcli storage nfs add -H <nfs-server> -s <export-path> -v <ds-name>
```

Add an **NFS v4.1** datastore (ESXi 6.0+):

```
esxcli storage nfs41 add -H <nfs-server> -s <export-path> -v <ds-name>
```

(Remove with `esxcli storage nfs{,41} remove -v <ds-name>`.) [TechDocs](https://techdocs.broadcom.com/us/en/vmware-cis/vsphere/vsphere/6-7/vsphere-storage-6-7/working-with-datastores-in-vsphere-storage-environment/nfs-datastore-concepts-and-operations-in-vsphere-environment/configuring-the-nfs-datastore/understand-the-esxcli-command-parameters-for-nfs.html?utm_source=chatgpt.com)[Server World+1](https://www.server-world.info/en/note?f=2&os=ESXi_8&p=storage&utm_source=chatgpt.com)

Rescan storage after changes (example for all HBAs):

```
esxcli storage core adapter rescan --all
```

[Nakivo](https://www.nakivo.com/blog/most-useful-esxcli-esxi-shell-commands-vmware-environment/?utm_source=chatgpt.com)

------

### TL;DR

- **View / minor ops:** `vim-cmd hostsvc/datastore/listsummary|info|rename|refresh`.
- **Add/remove/mount:** use **`esxcli`** (`storage filesystem`, `nfs`, `nfs41`, VMFS tools).
   That’s the normal split in ESXi: `vim-cmd` talks to the host API for inventory-ish tasks; `esxcli` does the plumbing. [reddit.com](https://www.reddit.com/r/vmware/comments/tvdvlr/difference_between_esxcli_and_vimcmd_hostsvc/?utm_source=chatgpt.com)

##### You said:

What if the DATASTORE is a LOCAL DISK ?

###### ChatGPT said:

Thought for 42s

Short version: when the datastore lives on a **local disk** (SATA/SAS/NVMe attached directly to the ESXi host), you still **view** it with `vim-cmd`, but you **configure** it with ESXi shell tools—mainly `partedUtil`, `vmkfstools`, and `esxcli`.

------

## View / inspect (works for local disks too)

```
# List all datastores (name/type/capacity)
vim-cmd hostsvc/datastore/listsummary

# Details on one datastore (e.g., forceMounted, capacity)
vim-cmd hostsvc/datastore/info <datastore-name>
```

These are read-only/management calls to hostd; they don’t create or format storage. [TechDocs](https://techdocs.broadcom.com/us/en/vmware-cis/vsphere/vsphere/6-7/vsphere-storage-6-7/working-with-datastores-in-vsphere-storage-environment/vsphere-vmfs-datastore-concepts-and-operations/create-a-vsphere-vmfs-datastore.html?utm_source=chatgpt.com)

------

## Create a VMFS datastore on an unused **local disk** (CLI)

> ⚠️ These steps are destructive to the selected disk. Double-check device IDs before running them.

1. **Identify the local device** and confirm it’s not in use:

```
# All storage devices; look for "Is Local: true"
esxcli storage core device list | less

# VMFS/NFS volumes currently known to the host
esxcli storage filesystem list
```

`esxcli storage core device list` shows flags like **Is Local: true** and the **/vmfs/devices/disks/** path you’ll use next. [Support Portal+1](https://knowledge.broadcom.com/external/article/316540/identifying-disks-when-working-with-vmwa.html?utm_source=chatgpt.com)

1. **Examine the partition table** (and note geometry):

```
partedUtil getptbl "/vmfs/devices/disks/<naa-or-mpx-id>"
```

If you need a fresh GPT + one VMFS partition that fills the disk, VMware’s doc shows using **start sector 2048** and the VMFS partition GUID. Example: [Support Portal](https://knowledge.broadcom.com/external/article/323144/using-partedutil-command-line-disk-parti.html)

```
# Create GPT with a single VMFS partition from sector 2048 to end
partedUtil setptbl "/vmfs/devices/disks/<naa-id>" gpt \
  "1 2048 <end-sector> AA31E02A400F11DB9590000C2911D1B8 0"
```

> Get `<end-sector>` from `getptbl` geometry (last usable sector). The VMFS type GUID is **AA31E02A400F11DB9590000C2911D1B8**. [Support Portal](https://knowledge.broadcom.com/external/article/323144/using-partedutil-command-line-disk-parti.html)

1. **Format the new partition as VMFS6**:

```
# Note the :1 (partition 1)
vmkfstools -C vmfs6 -S datastore-local \
  /vmfs/devices/disks/<naa-id>:1
```

This creates the VMFS on that partition with label `datastore-local`. [Support Portal](https://knowledge.broadcom.com/external/article/309687/manually-creating-a-vmfs-volume-using-vm.html)

1. **Rescan & verify**:

```
# Rescan all HBAs so the new datastore shows up
esxcli storage core adapter rescan --all

# Confirm it’s listed and mounted
esxcli storage filesystem list
```

[Support Portal](https://knowledge.broadcom.com/external/article/308546/performing-a-rescan-of-the-storage-on-an.html?utm_source=chatgpt.com)[VMware](https://www.vmware.com/docs/vsphere-80-u1-esxcli-storage-commands?utm_source=chatgpt.com)

If it didn’t auto-mount for some reason:

```
esxcli storage filesystem mount -l datastore-local
```

[VMware](https://www.vmware.com/docs/vsphere-80-u1-esxcli-storage-commands?utm_source=chatgpt.com)[VMware Download Center](https://vdc-download.vmware.com/vmwb-repository/dcr-public/82313c72-73fe-449a-ab22-d175a85e2dc4/8513152e-395d-4f27-9009-1dee3a49d47d/namespace/esxcli_storage.html?utm_source=chatgpt.com)

------

## Expand a local-disk datastore (after growing the disk/RAID)

1. Grow the partition to fill the new end sector:

```
# Example: keep start at 2048, set new last sector
partedUtil resize "/vmfs/devices/disks/<naa-id>" 1 2048 <new-end-sector>
```

[Support Portal](https://knowledge.broadcom.com/external/article/323144/using-partedutil-command-line-disk-parti.html)

1. Tell VMFS to grow into the free space:

```
# Pass the same partition twice (device:partition device:partition)
vmkfstools --growfs "/vmfs/devices/disks/<naa-id>:1" "/vmfs/devices/disks/<naa-id>:1"
```

(Rescan again and re-check with `esxcli storage filesystem list`.) [Support Portal](https://knowledge.broadcom.com/external/article/321398/failed-to-expand-vmfs-datastore-cannot.html?utm_source=chatgpt.com)

------

## Un/mount or rename (local or not)

```
# Rename via hostd API
vim-cmd hostsvc/datastore/rename <oldName> <newName>

# Unmount / mount a VMFS datastore by label or UUID
esxcli storage filesystem unmount -l <label>
esxcli storage filesystem mount   -l <label>
```

[Support Portal](https://knowledge.broadcom.com/external/article/381759/list-mount-and-unmount-a-volumevmfs-data.html?utm_source=chatgpt.com)

------

### Why not use `vim-cmd` to “make” a local datastore?

`vim-cmd` talks to hostd and is great for **inventory and metadata**. Disk-level changes (partitioning, formatting VMFS, mounting) are done by **`partedUtil` + `vmkfstools` + `esxcli`** on the host. That split is by design. [Support Portal+1](https://knowledge.broadcom.com/external/article/309687/manually-creating-a-vmfs-volume-using-vm.html)

If you want, I can tailor the exact commands for your disk (paste the output of `esxcli storage core device list | grep -E "Display Name|Devfs Path|Is Local|Size" -A4`).





why did vmware create two tools vim-cmd and esxcli ?

###### ChatGPT said:

Thought for 16s

Short answer: they solve **different layers** of the stack.

### Two layers, two CLIs

- **`vim-cmd`** talks to the **vSphere Web Services (VIM) API** through the host’s management service (**hostd**). It’s basically an API console for inventory-level tasks (VM power, registration, datastore metadata, host services). If `hostd` is down or overloaded, `vim-cmd` won’t work because it depends on that service. [TechDocs](https://techdocs.broadcom.com/us/en/vmware-cis/vsphere/vsphere-sdks-tools/8-0/an-introduction-getting-started-with-vsphere-apis-and-sdks-8-0/core-vsphere-apis/web-services-vim-api.html?utm_source=chatgpt.com)[Support Portal](https://knowledge.broadcom.com/external/article?legacyId=1038043&utm_source=chatgpt.com)
- **`esxcli`** talks to the **ESXi host subsystems** (storage, networking, hardware, drivers, PSA, etc.) via CLI namespaces. It’s the supported, unified replacement for older `esxcfg-*` tools and is designed for low-level host configuration and troubleshooting—locally or remotely. [VMware Download Center](https://vdc-download.vmware.com/vmwb-repository/dcr-public/a56728e8-1319-455a-ac30-01dd7a669654/c430f938-1e49-412f-8dbc-34a39712f683/GUID-3B627C99-2981-436B-B3C2-6AC482DC1431.html?utm_source=chatgpt.com)[Broadcom Developer](https://developer.broadcom.com/xapis/esxcli-command-reference/latest?utm_source=chatgpt.com)

### Why both exist

1. **Separation of concerns:**
   - *Management plane* (inventory/tasks) via VIM → `vim-cmd`.
   - *Control plane / host internals* (HBAs, VMFS, NICs, drivers) → `esxcli`.
      This keeps API/hostd operations distinct from direct host configuration. [TechDocs](https://techdocs.broadcom.com/us/en/vmware-cis/vsphere/vsphere-sdks-tools/8-0/an-introduction-getting-started-with-vsphere-apis-and-sdks-8-0/core-vsphere-apis/web-services-vim-api.html?utm_source=chatgpt.com)[Broadcom Developer](https://developer.broadcom.com/xapis/esxcli-command-reference/latest?utm_source=chatgpt.com)
2. **Operational resilience:**
    If `hostd` is unhealthy, `vim-cmd` fails; `esxcli` can still perform many host-level diagnostics/config (e.g., rescans, filesystem ops). [Support Portal](https://knowledge.broadcom.com/external/article?legacyId=1038043&utm_source=chatgpt.com)[Broadcom Developer](https://developer.broadcom.com/xapis/esxcli-command-reference/latest/namespace/esxcli_storage.html?utm_source=chatgpt.com)
3. **Support & stability:**
    `esxcli` is the **supported** command framework for ESXi administration, with stable namespaces and docs. `vim-cmd` is handy but primarily a troubleshooting/API surface, not the mainline config tool. [Broadcom Developer](https://developer.broadcom.com/xapis/esxcli-command-reference/latest?utm_source=chatgpt.com)[TechDocs](https://techdocs.broadcom.com/us/en/vmware-cis/vsphere/vsphere-sdks-tools/7-0/esxcli-concepts-and-examples-7-0.html?utm_source=chatgpt.com)
4. **Different audiences & workflows:**
   - API-minded admins/scripts (inventory, VM ops) → `vim-cmd`.
   - Host engineers/storage/network admins (drivers, VMFS/NFS, NICs) → `esxcli`. [StarWind HCI & VSAN](https://www.starwindsoftware.com/blog/cool-vsphere-command-line-interface-esxcli-commands-helped-lot/?utm_source=chatgpt.com)

### Handy mental model

- Need to **power on a VM, list VMs, rename a datastore**? → `vim-cmd` (VIM/hostd). [Support Portal](https://knowledge.broadcom.com/external/article?legacyId=1038043&utm_source=chatgpt.com)
- Need to **rescan HBAs, add NFS, inspect/format VMFS, tweak NIC/driver**? → `esxcli` (host subsystems). [Broadcom Developer](https://developer.broadcom.com/xapis/esxcli-command-reference/latest/namespace/esxcli_storage.html?utm_source=chatgpt.com)[VMware Download Center](https://vdc-download.vmware.com/vmwb-repository/dcr-public/a56728e8-1319-455a-ac30-01dd7a669654/c430f938-1e49-412f-8dbc-34a39712f683/GUID-3B627C99-2981-436B-B3C2-6AC482DC1431.html?utm_source=chatgpt.com)

If you tell me the operation you want, I’ll point you at the exact `vim-cmd` or `esxcli` incantation.





what tool can be used to create a VM?  Set devices set Disk Give it a name  etc

###### ChatGPT said:

Thought for 55s

Short version:

- **Best CLI for full VM creation:** `govc` (open-source vSphere CLI).
- **Deploy from templates/OVAs:** `ovftool`.
- **On an ESXi shell only:** `vim-cmd` + `vmkfstools` can do it, but it’s clunky.

Below are solid, copy-pasteable flows.

------

# 1) Create a VM with `govc` (recommended)

Works against ESXi or vCenter; lets you set CPU/RAM/NIC/disk in one go.

```
# 1) Point govc at your ESXi/vCenter
export GOVC_URL='https://<esxi-or-vcenter>/sdk'
export GOVC_USERNAME='root'            # or a vCenter user
export GOVC_PASSWORD='...'
export GOVC_INSECURE=1                 # if you don’t have valid TLS

# 2) Create the VM shell
govc vm.create -ds=datastore1 -on=false -c=2 -m=4096 \
  -g=other3xLinux64 -net="VM Network" -firmware=bios myvm

# 3) Add a virtual disk (thin 20G)
govc device.disk.create -vm=myvm -name=myvm/myvm.vmdk -size=20G

# 4) (Optional) Attach an ISO
govc device.cdrom.add -vm=myvm -iso='[datastore1] iso/ubuntu-24.04.iso'

# 5) Power on
govc vm.power -on myvm
```

`govc` is widely used and supports create/change/device subcommands (NICs, disks, etc.). [unixarena.com](https://www.unixarena.com/2019/06/govmomi-installing-and-configuring-govc-cli-for-vsphere.html/?utm_source=chatgpt.com)



# 2) Deploy an OVF/OVA with `ovftool`

If you already have an OVA (appliance or a golden image):

```
ovftool --name=myvm --datastore=datastore1 \
  --network="VM Network" ./myimage.ova \
  'vi://root:PASS@<esxi-or-vcenter>/'
```

OVF Tool is the official utility for importing/exporting OVF/OVAs. [Support Portal](https://knowledge.broadcom.com/external/article/340425/ovf-tool-command-syntax-to-export-and-de.html?utm_source=chatgpt.com)[VMware Download Center](https://vdc-download.vmware.com/vmwb-repository/dcr-public/eae00242-71e7-4c2b-ae3a-30733acd237f/444d1981-8d87-4efe-9c68-4b62b7082b01/ovftool-432-userguide.pdf?utm_source=chatgpt.com)

------

# 3) ESXi shell only: `vim-cmd` + `vmkfstools`

You can do it from the host’s SSH shell, but it’s more manual.

```
# 1) Create a bare “dummy” VM (hostd will make the folder & .vmx)
vim-cmd vmsvc/createdummyvm "myvm" /vmfs/volumes/datastore1
# note: some ESXi builds accept: createdummyvm <name> [datastore] /path/to/myvm.vmx

# 2) Get its VMID
vim-cmd vmsvc/getallvms

# 3) Create a VMDK (20G thin) in the VM’s folder
vmkfstools -c 20G -d thin /vmfs/volumes/datastore1/myvm/myvm.vmdk

# 4) Attach that existing disk (controller 0, unit 0)
vim-cmd vmsvc/device.diskaddexisting <VMID> /vmfs/volumes/datastore1/myvm/myvm.vmdk 0 0

# 5) (Verify devices / connect NIC if present)
vim-cmd vmsvc/device.getdevices <VMID>
```

- `createdummyvm` makes the skeleton VM; behavior varies slightly by ESXi version. [Server World](https://www.server-world.info/en/note?f=2&os=ESXi_7&p=vm&utm_source=chatgpt.com)[heelpbook.net](https://www.heelpbook.net/2017/vmware-esxi-vim-cmd-command/?utm_source=chatgpt.com)
- `vmkfstools` creates the virtual disk. [TechDocs](https://techdocs.broadcom.com/us/en/vmware-cis/vsphere/vsphere/6-5/vsphere-storage-6-5/using-vmkfstools-in-vsphere/vmkfstools-options/virtual-disk-options-of-vsphere-vmkfstools-command/creating-a-virtual-disk.html?utm_source=chatgpt.com)
- `vim-cmd vmsvc/device.diskaddexisting` (or `device.diskadd`) adds disks; `device.getdevices` shows hardware. [Gist](https://gist.github.com/jpluimers/8466191e3a01d36d3eff37bfe9da7191?utm_source=chatgpt.com)[The Engineer's Workshop](https://engineerworkshop.com/blog/how-to-add-a-hard-disk-to-an-esxi-virtual-machine/?utm_source=chatgpt.com)[Support Portal](https://knowledge.broadcom.com/external/article/369138/connecting-or-disconnecting-a-nic-for-a.html?utm_source=chatgpt.com)

> Note: Adding a **new** NIC via `vim-cmd` is awkward; most folks add NICs with `govc vm.network.add` or the UI, then use `vim-cmd vmsvc/device.getdevices` to inspect/attach/enable. [docs.stg.fedoraproject.org](https://docs.stg.fedoraproject.org/en-US/fedora-coreos/provisioning-vmware/?utm_source=chatgpt.com)

------

## Which should *you* use?

- **Automating builds / CI:** `govc` or `ovftool`.
- **Air-gapped ESXi host with only SSH:** `vim-cmd` + `vmkfstools` works, but is fiddly.
- **From your own hostd/vim-cmd strawman:** mimic the `vim-cmd` flow (create VM record, create VMDK, attach devices), or call the vSphere API like `govc` does.

If you tell me your exact target (ESXi vs. vCenter, ISO vs. OVA, desired CPU/RAM/disk/NIC), I’ll hand you the exact commands.