#ifndef MY_SPI_H
#define MY_SPI_H
#include <linux/spi/spi.h>
#include <linux/cdev.h>

struct omap2_mcspi_regs {
	u32 modulctrl;
	u32 wakeupenable;
};

struct omap2_mcspi {
	//struct spi_master	*master;
	/* Virtual base address of the controller */
	void __iomem		*base;
	unsigned long		phys;
	/* SPI1 has 4 channels, while SPI2 has 2 */
	struct device		*dev;
	struct omap2_mcspi_regs ctx;
	int			word_len;
	u32			chconf0;
	int			fifo_depth;
	unsigned int		pin_dir:1;
	/* for providing a character device access */
	struct class *spi_class;
	dev_t devt;
	struct cdev cdev;
};

int fcd_init(struct omap2_mcspi *mcspi);
void fcd_exit(void);
int omap2_mcspi_setup_transfer(struct omap2_mcspi *mcspi, struct spi_transfer *t);
void omap2_mcspi_cleanup(struct omap2_mcspi *mcspi);
int omap2_mcspi_setup(struct omap2_mcspi *mcspi);
int omap2_mcspi_transfer_one_message(struct omap2_mcspi *mcspi,
		struct spi_transfer *t);
int spi_rw(struct omap2_mcspi *mcspi, char *buff);

#endif
