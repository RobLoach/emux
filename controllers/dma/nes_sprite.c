#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <clock.h>
#include <controller.h>
#include <memory.h>
#include <resource.h>
#include <util.h>

/* DMA controller hard-coded destination address */
#define DEST_ADDRESS 0x2004

struct nes_sprite {
	int bus_id;
	struct region region;
};

static bool nes_sprite_init(struct controller_instance *instance);
static void nes_sprite_deinit(struct controller_instance *instance);
static void nes_sprite_writeb(struct nes_sprite *nes_sprite, uint8_t b,
	address_t address);

static struct mops nes_sprite_mops = {
	.writeb = (writeb_t)nes_sprite_writeb
};

void nes_sprite_writeb(struct nes_sprite *nes_sprite, uint8_t b,
	address_t UNUSED(address))
{
	uint16_t src_address;
	int i;

	/* Input byte represents upper byte of source address */
	src_address = b << 8;

	/* Transfer 256 bytes */
	for (i = 0; i < 256; i++) {
		b = memory_readb(nes_sprite->bus_id, src_address++);
		memory_writeb(nes_sprite->bus_id, b, DEST_ADDRESS);
	}

	/* The transfer takes 512 clock cycles and halts execution unit */
	clock_consume(512);
}

bool nes_sprite_init(struct controller_instance *instance)
{
	struct nes_sprite *nes_sprite;
	struct resource *area;

	/* Allocate nes_sprite structure */
	instance->priv_data = calloc(1, sizeof(struct nes_sprite));
	nes_sprite = instance->priv_data;

	/* Set up nes_sprite memory region */
	area = resource_get("mem",
		RESOURCE_MEM,
		instance->resources,
		instance->num_resources);
	nes_sprite->region.area = area;
	nes_sprite->region.mops = &nes_sprite_mops;
	nes_sprite->region.data = nes_sprite;
	memory_region_add(&nes_sprite->region);

	/* Save bus ID for later use */
	nes_sprite->bus_id = instance->bus_id;

	return true;
}

void nes_sprite_deinit(struct controller_instance *instance)
{
	free(instance->priv_data);
}

CONTROLLER_START(nes_sprite)
	.init = nes_sprite_init,
	.deinit = nes_sprite_deinit
CONTROLLER_END

