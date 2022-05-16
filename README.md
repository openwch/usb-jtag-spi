480Mbps High Speed USB2.0 JTAG Debugger   Open source information
  |-- bin
  |      |--MCU: MCU target program
  |      |--WIN APP
  |               |--USB20Jtag.exe：USB2.0 to JTAG interface debugging tool
  |               |--USB20SPI.exe：USB2.0 to SPI interface debugging tool
  |               |--OpenOCD: Host computer JTAG debugger OpenOCD, support FPGA XC6S
  |
  |-- doc
  |      |-- USB to JTAG-SPI interface communication protocol
  |
  |-- driver
  |      |--USB20_DRIVER: USB Windows driver, supports USB FS(12MBps)/HS(480MBps)/SS(5GBps), supports WIN2000/XP, 32/64-bit WIN7/8/8.1/10/11 operating system.
  |                                  Download link: http://www.wch.cn/downloads/CH372DRV_EXE.html
  |-- sch   
  |      |-- Reference schematic
  |   
  |-- src
  |      |-- MCU: Based on CH32V305 series chip MCU firmware source code
  |      |-- OpenOCD: Host computer JTAG debugger OpenOCD source code, support FPGA XC6S
  |      |-- WIN APP: USB2.0 to JTAG source code
  |               |-- USB20Jtag : USB2.0 to JTAG interface host computer example program, C++/VS2008
  |               |-- USB20SPI  : USB2.0 to SPI interface host computer sample program, C++/VS2008
  |
  |-- tools
  |      |-- MCU IDE(MounRiver)：http://www.mounriver.com/download
  |      |-- MCU ISP Tool：http://www.wch.cn/downloads/WCHISPTool_Setup_exe.html

  
