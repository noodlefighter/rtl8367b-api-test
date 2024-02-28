#include "rtk_api.h"
#include "rtl8367b_asicdrv_port.h"
#include "rtl8367b_asicdrv_vlan.h"
#include "rtl8367b_asicdrv_lut.h"
#include "rtk_api_ext.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#define DUMP_FILE "rtl8367_eeprom.bin"

#define DEFAULT_VALUE 0xFFFF

uint16_t dummy_regs[0xFFFF];


void msleep(int)
{
}

void setReg(rtk_uint16 reg, rtk_uint16 val)
{
    dummy_regs[reg] = val;
}
rtk_uint16 getReg(rtk_uint16 reg)
{
    return dummy_regs[reg];
}

void regs_init()
{
    int i;
    for(i = 0; i < sizeof(dummy_regs)/sizeof(dummy_regs[0]); i++)
    {
        dummy_regs[i] = DEFAULT_VALUE;
    }
}
static void asic_vlan_set_ingress_ports(rtk_uint32 reg_ingress)
{
 #define REG_VLAN_INGRESS			RTL8367B_REG_VLAN_INGRESS
	setReg(REG_VLAN_INGRESS, reg_ingress);
}

void regs_dump()
{
    int i;
    unsigned char buf[4];

    FILE *fp = fopen(DUMP_FILE, "w");
    if (fp == NULL) {
        fprintf(stderr, "Can't open %s\n", DUMP_FILE);
        exit(1);
    }


    /* RTL8367写EEPROM的格式:
        前两个字节（一个uint16数据）表示eeprom中数据长度
        后面每组数据内容为: 寄存器地址（uint16） 数据（uint16）
    */

    // 长度
    {
        int reg_num = 0;
        int datasize;

        // 有多少个需要dump出的寄存器？
        for(i = 0; i < sizeof(dummy_regs)/sizeof(dummy_regs[0]); i++)
        {
            if (dummy_regs[i] != DEFAULT_VALUE) {
                reg_num++;
            }
        }
        datasize = 2 + reg_num*4;

        buf[0] = (datasize) & 0xff;
        buf[1] = (datasize >> 8) & 0xff | 0x80;
        fwrite(buf, 2, 1, fp);
    }
    // 数据
    for(i = 0; i < sizeof(dummy_regs)/sizeof(dummy_regs[0]); i++)
    {
        if (dummy_regs[i] != DEFAULT_VALUE) {
            printf("%04X: 0x%04X\n", i, dummy_regs[i]);

            buf[0] = (i) & 0xFF;
            buf[1] = (i >> 8) & 0xFF;
            buf[2] = (dummy_regs[i]) & 0xFF;
            buf[3] = (dummy_regs[i] >> 8) & 0xFF;
            fwrite(buf, 4, 1, fp);
        }
    }
}

#define PORT_1    0
#define PORT_2    1
#define PORT_3    2
#define PORT_4    3
// #define PORT_CPU  4
#define PORT_CPU  PORT_3
#define PORT_MAX  PORT_CPU

#define BIT_MASK(n)  (1 << n)

static void set_vlan(rtk_uint32 cvid, rtk_uint32 member_mask, rtk_uint32 untag_mask)
{
	rtk_portmask_t mask_member, mask_untag;
    mask_member.bits[0] = member_mask;
    mask_untag.bits[0] = untag_mask;
	rtk_vlan_set(cvid, mask_member, mask_untag, 1);
}
#define RTL8367_DEVNAME "rtl8367"
static void asic_dump_isolation(void)
{
	int i, ivl_svl;
	rtk_api_ret_t retVal;
	rtk_data_t Efid;
	rtk_vlan_t Pvid;
	rtk_pri_t Priority;
	rtk_fid_t Fid;
	rtk_portmask_t mask1, mask2;
	rtk_enable_t Igr_filter;
	rtk_vlan_acceptFrameType_t Accept_frame_type;

	printf("%s: dump ports isolation:\n", RTL8367_DEVNAME);

	for (i = 0; i < RTK_MAX_NUM_OF_PORT; i++) {
		rtk_port_efid_get(i, &Efid);
		retVal = rtk_port_isolation_get(i, &mask1);
		if (retVal == RT_ERR_OK)
			printf("  port (%d) isolation: mask=%04X, efid=%d\n", i, mask1.bits[0], Efid);
	}

	printf("%s: dump port-based vlan:\n", RTL8367_DEVNAME);

	for (i = 0; i < RTK_MAX_NUM_OF_PORT; i++) {
		retVal = rtk_vlan_portPvid_get(i, &Pvid, &Priority);
		if (retVal == RT_ERR_OK)
			printf("  port (%d) vlan: pvid=%d, prio=%d\n", i, Pvid, Priority);
	}

	printf("%s: dump ports accept mode:\n", RTL8367_DEVNAME);

	for (i = 0; i < RTK_MAX_NUM_OF_PORT; i++) {
		rtk_vlan_portIgrFilterEnable_get(i, &Igr_filter);
		retVal = rtk_vlan_portAcceptFrameType_get(i, &Accept_frame_type);
		if (retVal == RT_ERR_OK)
			printf("  port (%d) accept: type=%d, ingress=%d\n", i, Accept_frame_type, Igr_filter);
	}

	printf("%s: dump vlan members:\n", RTL8367_DEVNAME);

	for (i = 1; i < 15; i++) {
        retVal = rtk_vlan_get(i, &mask1, &mask2, &Fid);
        if (retVal == RT_ERR_OK) {
            ivl_svl = 0;
            printf("  vlan (%d): member=%04X, untag=%04X, fid=%d, ivl=%d\n", i, mask1.bits[0], mask2.bits[0], Fid, ivl_svl);
        }
	}
}

int main()
{
    rtk_portmask_t mask;
    rtk_fid_t fid = 1;

    regs_init();

	rtk_switch_init();

    mask.bits[0] = BIT_MASK(PORT_1) | BIT_MASK(PORT_CPU);
    rtk_port_isolation_set(PORT_1, mask);
    mask.bits[0] = BIT_MASK(PORT_2) | BIT_MASK(PORT_CPU);
    rtk_port_isolation_set(PORT_2, mask);
    mask.bits[0] = BIT_MASK(PORT_3) | BIT_MASK(PORT_CPU);
    rtk_port_isolation_set(PORT_3, mask);

    rtk_port_phyEnableAll_set(ENABLED);

    // VLAN table
    rtk_vlan_init();
    // {
    //     rtk_portmask_t mask_member, mask_untag;
    //     uint32_t ports_mask = BIT_MASK(PORT_1) | BIT_MASK(PORT_2) | BIT_MASK(PORT_CPU);
    //     // vlan 1
    //     mask_untag.bits[0] = mask_member.bits[0] = ports_mask;

    //     if (RT_ERR_OK != rtk_vlan_set(1, mask_member, mask_untag, fid)) {
    //         fprintf(stderr, "rtk_vlan_set 1 failed\n");
    //     }
    //     for (int i = 2; i < 4095; i++) {
    //         mask_member.bits[0] = 0;
    //         mask_untag.bits[0]  = 0;
    //         if (RT_ERR_OK != rtk_vlan_set(i, mask_member, mask_untag, 0)) {
    //             fprintf(stderr, "rtk_vlan_set %d failed\n", i);
    //         }
    //     }

    //     /* set ingress filtering */
    //     asic_vlan_set_ingress_ports(ports_mask);

        for (int i = 0; i <= PORT_MAX; i++)
            rtk_vlan_portAcceptFrameType_set(i, ACCEPT_FRAME_TYPE_ALL);

    // }

    set_vlan(1, 0, 0);

    set_vlan(11, BIT_MASK(PORT_1) | BIT_MASK(PORT_CPU), BIT_MASK(PORT_1));
    set_vlan(12, BIT_MASK(PORT_2) | BIT_MASK(PORT_CPU), BIT_MASK(PORT_2));
    // set_vlan(13, BIT_MASK(PORT_3) | BIT_MASK(PORT_CPU), BIT_MASK(PORT_3));
    // set_vlan(14, BIT_MASK(PORT_4) | BIT_MASK(PORT_CPU), BIT_MASK(PORT_4));

    // PVID
	rtk_vlan_portPvid_set(PORT_1, 11, fid);
	// rtk_vlan_portAcceptFrameType_set(PORT_1, ACCEPT_FRAME_TYPE_ALL);
    rtk_vlan_portPvid_set(PORT_2, 12, fid);
    // rtk_vlan_portAcceptFrameType_set(PORT_2, ACCEPT_FRAME_TYPE_ALL);
    // // rtk_vlan_portPvid_set(PORT_3, 13, 0);
    // // rtk_vlan_portAcceptFrameType_set(PORT_3, ACCEPT_FRAME_TYPE_ALL);
    // // rtk_vlan_portPvid_set(PORT_4, 14, 0);
    // // rtk_vlan_portAcceptFrameType_set(PORT_4, ACCEPT_FRAME_TYPE_ALL);


    regs_dump();
    asic_dump_isolation();
    return 0;
}
