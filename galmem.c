#include <generated/autoconf.h>
#include <linux/kernel.h> /* printk() */
#include <linux/module.h>
#include <linux/pci.h>
#include <linux/init.h>
#include <linux/errno.h>	/* error codes */
#include <linux/io.h>

MODULE_AUTHOR("Remco Terwal <remcoterwal_AT_rcn.com>");
MODULE_DESCRIPTION("Galateo Memory");
MODULE_LICENSE("GPL");

/*PCI_VENDOR_ID*/
#ifndef PCI_VENDOR_ID_XILINX
#define PCI_VENDOR_ID_XILINX    0x10ee
#endif

/*PCI_DEVICE_ID*/
#ifndef PCI_DEVICE_ID_XILINX
#define PCI_DEVICE_ID_XILINX    0x0007
#endif


static const struct pci_device_id galmem_ids[] = {
	{.vendor = PCI_VENDOR_ID_XILINX,
	 .device = PCI_DEVICE_ID_XILINX,
	 .subvendor = PCI_VENDOR_ID_XILINX,
	 .subdevice = PCI_DEVICE_ID_XILINX,
	 .class = 0,
	 .class_mask = 0,
	 .driver_data = 0},
	{0,} /* end of table */
};
MODULE_DEVICE_TABLE(pci, galmem_ids);

static unsigned char galmem_get_revision(struct pci_dev *dev)
{
	u8 revision;

	pci_read_config_byte(dev, PCI_REVISION_ID, &revision);
	return revision;
}

static int galmem_probe(struct pci_dev *dev, const struct pci_device_id *id)
{
	int err;
    resource_size_t mmio_start; /* bar0 phsyical address */
    resource_size_t mmio_len; /* bar0 memoroy size at bar0 */
    void __iomem *bar0_base; /* bar0 virtual address */

	err = pci_enable_device(dev);
    if (err) {
        printk(KERN_ALERT "Cannot enable Galateo PCIe device\n");
        return err;
    }

    /* Report the Galateo hardware revision */
    printk(KERN_ALERT "Galateo revision ID = 0x%02X\n", galmem_get_revision(dev));

    /* Request MMIO resources for BAR0 */
    /* Use pci_request_selected_regions() for all or a specific BAR */
    err = pci_request_region(dev, 0, "galmem");
    if (err) {
        pci_disable_device(dev);
        return err;
    }
    
    /* Obtain the size of the BAR0 mapped memory from the PCIe header. */
    /* Size of the memory = dword*length */
    mmio_len = pci_resource_len(dev, 0);
    /* Map a MMIO region to a kernel virtual address mapped to physical address BAR0*/
    /* A 0 is passed for the start address to allow the OS prvision the virtual address */
    bar0_base = pci_iomap(dev, 0, mmio_len);
    if (!bar0_base) { /* A NULL return value is success! */
        pci_release_region(dev, 0);
        pci_disable_device(dev);
        return -ENOMEM;
    }

    /* Get the physical start address of BAR 0 out of the PCIe header; for information & verification only */
    mmio_start = pci_resource_start(dev, 0);
    dev_info(&dev->dev, "BAR 0 physical address start: 0x%pa, length 0x%llx\n", &mmio_start, mmio_len);

    /* Store BAR0_base virtual address in private data in the pci_dev structure; Avoid global variable! */
    pci_set_drvdata(dev, bar0_base);

    /* Accessing a memory location (e.g., reading a 32-bit location at an offset) */
    u32 value = ioread32(bar0_base + 0x8);
    dev_info(&dev->dev, "Read value 0x%x from register\n", value);

	return 0;
}

static void galmem_remove(struct pci_dev *dev)
{
    void __iomem *bar0_base = pci_get_drvdata(dev); /* retrieve bar0 virtual address */

    if (bar0_base) 
    {
        pci_iounmap(dev, bar0_base);
    }
    pci_release_region(dev, 0);
    pci_disable_device(dev);
}

static struct pci_driver pci_driver = {
	.name = "galmem",
	.id_table = galmem_ids,
	.probe = galmem_probe,
	.remove = galmem_remove,
};

static int __init galmem_init(void)
{
	printk(KERN_ALERT "Registering galmem driver...\n");
	return pci_register_driver(&pci_driver);
}

static void __exit galmem_exit(void)
{
	printk(KERN_ALERT "Unregistering galmem driver...\n");
	pci_unregister_driver(&pci_driver);
}

module_init(galmem_init);
module_exit(galmem_exit);
