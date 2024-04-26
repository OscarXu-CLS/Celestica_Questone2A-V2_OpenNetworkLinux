import subprocess
import time
from onl.platform.base import *
from onl.platform.celestica import *
import tempfile
import subprocess
import re

class OnlPlatform_x86_64_cel_ds2000_r0(OnlPlatformCelestica,
                                            OnlPlatformPortConfig_48x25_8x100):
    PLATFORM='x86-64-cel-ds2000-r0'
    MODEL="ds2000"
    SYS_OBJECT_ID=".2060.1"
    
    @staticmethod
    def send_cmd(cmd):
        try:
            data = subprocess.check_output(cmd, shell=True,
                                           universal_newlines=True, stderr=subprocess.STDOUT)
            sta = True
        except subprocess.CalledProcessError as ex:
            data = ex.output
            sta = False
        if data[-1:] == '\n':
            data = data[:-1]
        return sta, data

    def change_baud(self, file_path, baud):
        grub_path = file_path + "/grub/grub.cfg"
        with open(grub_path, "r") as f:
            grub_info = f.read()
            speed_find = re.findall("--speed=(\\d+)", grub_info)
            console_find = re.findall("console=ttyS0,(\\d+)n8", grub_info)
            if not speed_find or not console_find:
                return False
            speed = speed_find[0].strip()
            console = console_find[0].strip()
            if speed != console:
                return False
            change_baud_cmd = "sed -i s/%s/%s/ %s" % (speed, baud, grub_path)
            status_, res_ = self.send_cmd(change_baud_cmd)
            return status_, res_

    def run_command(self, cmd):
        status = True
        result = ""
        try:
            p = subprocess.Popen(
                cmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
            raw_data, err = p.communicate()
            if err == '':
                result = raw_data.strip()
        except:
            status = False
        return status, result

    def register_hwdevice_multi_times(self, driver, bus, addr, times, node):
        """
        Register the hwmon device multiple times to fix loading issue
        Param:
            string: driver name like "fsp550"
            int:    bus, decimal format like 11
            hex:    addr, hex format like 0x59
            string: node like 'fan1_input'
        Returns:
            bool:   true for success , false for fail
        """
        count = 0
        while count < times:
            self.new_i2c_device(driver, addr, bus)
            ret = os.system("ls /sys/bus/i2c/devices/i2c-%d/%d-%4.4x/hwmon/hwmon*/ | grep %s > /dev/null" % (bus, bus, addr, node))
            if ret == 0:
                return True
            os.system("echo 0x%4.4x > /sys/bus/i2c/devices/i2c-%d/delete_device" % (addr, bus))
            count = count + 1
        return False

    def baseconfig(self):
        sfp_quantity = 48
        sfp_i2c_start_bus = 13
        qsfp_quantity = 8
        qsfp_i2c_start_bus = 61
        sfp_pca9548_quantity = 6
        sfp_pca9548_first_index = 2
        qsfp_pca9548_quantity = 1
        qsfp_pca9548_index = 4
        qsfp_bus_map = [65,66,67,68,61,62,63,64,]
        fan_quantity = 4
        fan_bus_map = [81,80,78,77,]

        #Celestica Blacklist file
        blacklist_file_path="/etc/modprobe.d/celestica-blacklist.conf"
        #Blacklist the unuse module.
        if os.path.exists(blacklist_file_path):
            os.system("rm {0}".format(blacklist_file_path))
       
        os.system("touch {0}".format(blacklist_file_path))
        cel_paths = "/lib/modules/{0}/onl/celestica/".format(os.uname()[2])
        cel_dirs = os.listdir(cel_paths)
        for dir in cel_dirs:
            full_cel_path=cel_paths+dir
            if os.path.isdir(full_cel_path):
                modules=os.listdir(full_cel_path)
                for module in modules:
                    os.system("echo 'blacklist {0}' >> {1}".format(module[0:-3],blacklist_file_path))

        # rootfs overlay
        stamp_dirname = os.path.dirname(__file__)

        if not os.path.exists(stamp_dirname + "/rootfs_overlay.stamp"):
            os.system("cp -r " + stamp_dirname + "/rootfs_overlay/* /")
            os.system("sync /")
            os.system("touch " + stamp_dirname + "/rootfs_overlay.stamp")

        print("Initialize and Install the driver here")
        os.system("modprobe i2c-ismt")
        self.insmod("fpga_device.ko")
        self.insmod("fpga_i2c_ocores.ko")
        self.insmod("fpga_system.ko")
        self.insmod("i2c_switchcpld.ko")
        self.insmod("lpc_basecpld.ko")
        self.insmod("watch_dog.ko")

        # Add drivers
        os.system("modprobe optoe")

        # Add tlv eeprom device
        self.new_i2c_device('24c64', 0x56, 1)

        # Add PCA9548
        self.new_i2c_device('pca9548', 0x72, 2)
        self.new_i2c_device('pca9548', 0x73, 2)
        self.new_i2c_device('pca9548', 0x74, 2)
        self.new_i2c_device('pca9548', 0x75, 2)
        self.new_i2c_device('pca9548', 0x76, 2)
        self.new_i2c_device('pca9548', 0x77, 2)
        self.new_i2c_device('pca9548', 0x74, 3)

        # Add switch CPLD
        self.new_i2c_device("switchboard", 0x30, 4)
        self.new_i2c_device("switchboard", 0x31, 4)

        print("Running Watchdog")
        os.system("nohup python -u /usr/lib/python2.7/dist-packages/onl/platform/x86_64_cel_ds2000_r0/wdt/wdt_control.py > \
                   /usr/lib/python2.7/dist-packages/onl/platform/x86_64_cel_ds2000_r0/wdt/wdt_control.log 2>&1 &")
        
        # initialize SFP&QSFP devices name
        for actual_i2c_port in range(sfp_i2c_start_bus, sfp_i2c_start_bus+sfp_quantity):
            self.new_i2c_device('optoe2', 0x50, actual_i2c_port)
            sfp_port_number = actual_i2c_port - (sfp_i2c_start_bus-1)
            os.system("echo 'SFP{1}' > /sys/bus/i2c/devices/i2c-{0}/{0}-0050/port_name".format(actual_i2c_port,sfp_port_number))
            
        #for actual_i2c_port in range(qsfp_i2c_start_bus, qsfp_i2c_start_bus+qsfp_quantity):
        for qsfp_index in range(qsfp_quantity):
            self.new_i2c_device("optoe1", 0x50, qsfp_bus_map[qsfp_index])
            os.system("echo 'QSFP{1}' > /sys/bus/i2c/devices/i2c-{0}/{0}-0050/port_name".format(qsfp_bus_map[qsfp_index],qsfp_index + 1))

        # disconnect the SFP-PCA9548 idle,set it to -2.
        for sfp_pca9548_index in range(sfp_pca9548_first_index, (sfp_pca9548_first_index + sfp_pca9548_quantity)):
            pca_sfp_cmd = "echo -2 > /sys/bus/i2c/devices/2-007%d/idle_state" %sfp_pca9548_index
            os.system(pca_sfp_cmd)

        # disconnect the QSFP-PCA9548 idle,set it to -2.
            pca_qsfp_cmd = "echo -2 > /sys/bus/i2c/devices/3-007%d/idle_state" %qsfp_pca9548_index
	    os.system(pca_qsfp_cmd)

        os.system("echo 4 > /proc/sys/kernel/printk")

        # Automatically adapt to baud rate
        actual_baud_rate = os.popen("stty -F /dev/ttyS0 | awk '{print $2}' | sed -n 1p").read().strip()
        check_br_cmd = "grep -n '%s' /mnt/onl/boot/grub/grub.cfg" % actual_baud_rate
        check_res = os.system(check_br_cmd)
        if check_res != 0:
            sta, tmp_dir = self.send_cmd("mktemp -d -t grub-XXX")
            if sta:
                print('created temporary directory', tmp_dir)
            self.send_cmd("umount /dev/sda3")
            self.send_cmd("mount /dev/sda3 %s" % tmp_dir)
            status, res = self.change_baud(tmp_dir, actual_baud_rate)
            self.send_cmd("umount /dev/sda3")
            self.send_cmd("mount -o ro /dev/sda3 /mnt/onl/boot/")
            self.send_cmd("rm -rf %s" % tmp_dir)

            if not status:
                print("Change Baud Failed: %s" % res)

        # Check BMC present status
        os.system("echo 0xA108 > /sys/bus/platform/devices/sys_cpld/getreg")
        result = os.popen("cat /sys/bus/platform/devices/sys_cpld/getreg").read()

        if "0x00" in result or "0x02" in result:
            print("BMC is present")
            
            # initialize onlp cache files
            print("Initialize ONLP Cache files")
            os.system(
                "ipmitool fru > /tmp/onlp-fru-cache.tmp; sync; rm -f /tmp/onlp-fru-cache.txt; mv /tmp/onlp-fru-cache.tmp /tmp/onlp-fru-cache.txt"
            )
            os.system(
                "ipmitool sensor list > /tmp/onlp-sensor-list-cache.tmp; sync; rm -f /tmp/onlp-sensor-list-cache.txt; mv /tmp/onlp-sensor-list-cache.tmp /tmp/onlp-sensor-list-cache.txt"
            )
            
        elif "0x01" in result or "0x03" in result:
            print("BMC is absent")

            self.insmod("mp2975.ko")
            self.insmod("platform_psu.ko")
            self.insmod("platform_fan.ko")

            os.system("modprobe lm75")
            os.system("modprobe ucd9000")
            os.system("modprobe jc42")


            # COMe eeprom
            self.new_i2c_device("24c64", 0x50, 5)

            # Switchboard eeprom
            self.new_i2c_device("24c64", 0x50, 6)

            # Baseboard eeprom
            self.new_i2c_device("24c64", 0x57, 6)

            # Base CPLD
            self.new_i2c_device("platform_fan", 0x0d, 7)

            # PSU Bus 69-76
            self.new_i2c_device("pca9548", 0x70, 8)

            # UCD90120
            self.new_i2c_device("ucd90120", 0x34, 9)
            self.new_i2c_device("ucd90120", 0x35, 9)

            # MP2975 VR Power
            self.new_i2c_device("mp2975", 0x70, 10)
            self.new_i2c_device("mp2975", 0x7a, 10)

            # LM75B
            self.new_i2c_device("lm75b", 0x49, 11)
            self.new_i2c_device("lm75b", 0x4a, 11)
            self.new_i2c_device("lm75b", 0x4b, 11)
            self.new_i2c_device("lm75b", 0x4c, 11)
            self.new_i2c_device("lm75b", 0x4d, 11)
            self.new_i2c_device("lm75b", 0x4e, 11)

            # Fan Bus 77-84
            self.new_i2c_device("pca9548", 0x77, 12)

            # disconnect the FAN-PCA9548 idle,set it to -2.
            fan_cmd = "echo -2 > /sys/bus/i2c/devices/12-0077/idle_state"
            os.system(fan_cmd)

            # Fan eeprom(CH0-Fan4;CH1-Fan3;CH3-Fan2;CH4-Fan1)
            for fan_index in range(fan_quantity):
                self.new_i2c_device("24c64", 0x50, fan_bus_map[fan_index])

            # PSU eeprom
            self.new_i2c_device("24c02", 0x52, 69)
            self.new_i2c_device("fsp550", 0x5a, 69)
            self.new_i2c_device("24c02", 0x53, 70)
            self.new_i2c_device("fsp550", 0x5b, 70)

        return True

