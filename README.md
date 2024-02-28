via: https://github.com/Yonsm/rt-n56u

trunk/linux-3.4.x/drivers/net/rtl8367

refer里是一起扒下来的驱动，作为调用API的参考

情况说明

1. 只是把上面那仓库里的部分API扒了下来, 尝试把改动部分的数据下载到eeprom中，并不能配置成功RTL8367RB的VLAN功能

2. 想把SMI实现了，直接用GPIO控制8367看能不能成功，待做，所以留个档

参考

- https://github.com/tomazas/RTL8XXX-Switch

- https://github.com/shiroichiheisen/Realtek-Unmanaged-Switch-Arduino-Library
