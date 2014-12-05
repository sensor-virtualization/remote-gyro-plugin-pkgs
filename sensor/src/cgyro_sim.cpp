/*
 * emulator-plugin-gyro-pkgs
 *
 * Copyright (c) 2000 - 2011 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact: 
 * SooYoung Ha <yoosah.ha@samsnung.com>
 * Sungmin Ha <sungmin82.ha@samsung.com>
 * DongKyun Yun <dk77.yun@samsung.com>
 * 
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the
 * Free Software Foundation; either version 2.1 of the License, or (at your option)
 * any later version.
 * 
 * This library is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, write to the Free Software Foundation, Inc., 51
 * Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 *
 * Contributors:
 * - S-Core Co., Ltd
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <dlfcn.h>
#include <pthread.h>
#include <string.h>
#include <sys/un.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>

#include <common.h>
#include <cobject_type.h>
#include <cmutex.h>
#include <clist.h>
#include <cmodule.h>
#include <cpacket.h>
#include <csync.h>
#include <cworker.h>
#include <csock.h>
#include <sf_common.h>

#include <csensor_module.h>

#include <cgyro_sim.h>
#include <l3g4200d.h>

#ifdef _DEBUG
	#ifdef LOG_TAG
		#undef LOG_TAG
	#endif
	#define LOG_TAG	"gyro_sim_SENSOR"
#endif

#define SENSOR_NAME	"Emul_ModelId_Gyro"
#define SENSOR_VENDOR	"Emul_Vendor"

const char *cgyro_sim::m_port[] = {"x", "y", "z"};

cgyro_sim::cgyro_sim()
: m_name(NULL)
, m_resource(NULL)
, m_id(0x00200002)
, m_version(1)
, m_polling_interval(100000)
, m_x(-1)
, m_y(-1)
, m_z(-1)
, m_fired_time(0)
, m_client(0)
, m_sensor_type(ID_GYROSCOPE)
, m_ioctl_fd(-1)
, m_sensitivity(L3G4200D_FS_500DPS_SENSITIVITY)
, m_hw_type_k3g(false)
{
	m_name = strdup("gyro_sim");
	m_resource = strdup("/opt/sensor/gyro");
	if ((!m_name) ||(!m_resource)) {
		free(m_name);
		free(m_resource);
		throw ENOMEM;
	}
	
}



cgyro_sim::~cgyro_sim()
{
	free(m_name);
	free(m_resource);
	if ( m_ioctl_fd != -1 ) {
		close(m_ioctl_fd);
	}
		
}



const char *cgyro_sim::name(void)
{
	return m_name;
}



int cgyro_sim::version(void)
{
	return m_version;
}



int cgyro_sim::id(void)
{
	return m_version;
}



bool cgyro_sim::update_value(void)
{
	FILE *fp;
	int state;
	long  gyro_raw[3];
	char raw_data_node[256] = "/dev/virtual_sensor";
	
	fp = fopen(raw_data_node, "r");
	if (!fp){
		ERR("Failed to open sensor device : %s\n", raw_data_node);
		return false;
	}
	
	if (fscanf(fp, "%d %d %d", &gyro_raw[0], &gyro_raw[1], &gyro_raw[2]) != 1){
		ERR("Failed to collect data from : %s",raw_data_node);
		fclose(fp);
		return false;
	}
	fclose(fp);
	csensor_module::lock();
	m_x = gyro_raw[0];
	m_y = gyro_raw[1];
	m_z = gyro_raw[2];		
	csensor_module::unlock();	

	DBG("Update done raw : %d, %d, %d , out_data : %d, %d, %d\n", gyro_raw[0],gyro_raw[1],gyro_raw[2],m_x, m_y, m_z);
	return true;
}



bool cgyro_sim::is_data_ready(bool wait)
{
	unsigned long long cur_time;
	unsigned long elapsed_time;
	struct timeval sv;
	bool ret = false;

	DbgPrint("Sensor, invoked\n");
	gettimeofday(&sv, NULL);
	cur_time = MICROSECONDS(sv);

	elapsed_time = (unsigned long)(cur_time - m_fired_time);
	if (elapsed_time < m_polling_interval) {
		DbgPrint("Waiting\n");
		if (wait) {
			/*
			struct timespec ts;
			ts.tv_sec = 0;
			ts.tv_nsec= (m_polling_interval - elapsed_time) * 1000llu;
			nanosleep(&ts, NULL);
			*/
			usleep(m_polling_interval - elapsed_time);
			csensor_module::lock();
			m_fired_time = cur_time + (m_polling_interval-elapsed_time);
			csensor_module::unlock();
			ret = update_value();
		} else {
			ret = true;
		}
	} else {
		DbgPrint("Re-firing %llu %llu\n", cur_time, m_fired_time);
		DbgPrint("elapsed_time %lu polling_interval %ld\n", elapsed_time, m_polling_interval);
		csensor_module::lock();
		m_fired_time = cur_time;
		csensor_module::unlock();
		ret = update_value();
	}


	return ret;
}



long cgyro_sim::value(const char *port)
{

	if (!strcasecmp(port, "x")) {
		return m_x;
	} else if (!strcasecmp(port, "y")) {
		return m_y;
	} else if (!strcasecmp(port, "z")) {
		return m_z;
	}

	return -1;
}



long cgyro_sim::value(int id)
{

	if (id == 0) {
		return m_x;
	} else if (id == 1) {
		return m_y;
	} else if (id == 2) {
		return m_z;
	}

	return -1;
}


void cgyro_sim::reset(void)
{
	return;
}



bool cgyro_sim::update_name(char *name)
{
	char *new_name;
	new_name = strdup(name);
	if (!new_name) {
		DbgPrint("No memory\n");
		return false;
	}

	free(m_name);
	m_name = new_name;
	return true;
}



bool cgyro_sim::update_version(int version)
{
	m_version = version;
	return true;
}



bool cgyro_sim::update_id(int id)
{
	m_id = id;
	return true;
}



int cgyro_sim::port_count(void)
{
	return 3;
}



const char *cgyro_sim::port(int idx)
{
	if (idx >= (int)(sizeof(m_port)/sizeof(const char*))) {
		return NULL;
	}

	return m_port[idx];
}



bool cgyro_sim::need_polling(void)
{
	return m_polling_interval != 0;
}



long cgyro_sim::polling_interval(void)
{
	return (unsigned long long)m_polling_interval /1000llu ;
}



bool cgyro_sim::update_polling_interval(unsigned long val)
{
	DbgPrint("Update polling interval %lu\n", val);
	csensor_module::lock();
	m_polling_interval = (unsigned long long)val * 1000llu;
	csensor_module::unlock();
	return true;
}



bool cgyro_sim::start(void)
{

	m_client ++;

	if (m_client > 1) {
		return true;
	}
/*
	if ( !m_hw_type_k3g ) {
	//gyro_sim does not need init-state
		m_ioctl_fd = open(m_resource , O_RDWR);
		if ( m_ioctl_fd < 0 ) {
			ERR("Cannot open ioctl node : %s for ST_gyro\n",m_resource);
			return false;
		}
	}
*/
	return true;
}



bool cgyro_sim::stop(void)
{

	m_client --;
	if (m_client > 0) {
		DbgPrint("%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% Stopping\n");
		return true;
	}
/*
	if ( !m_hw_type_k3g ) {
	//gyro_sim does not need final-state
		if (m_ioctl_fd < 0 ) {
			ERR("m_ioctl_fd value check fail : %d\n", m_ioctl_fd);
			return false;
		}

		close(m_ioctl_fd);
		m_ioctl_fd = -1;
	}
*/
	return true;
}

int cgyro_sim::get_sensor_type(void)
{
	return m_sensor_type;
}

long cgyro_sim::set_cmd(int type , int property , long input_value)
{
	long value = -1;

#if 0
	if ( type == m_sensor_type) {
		switch (property) {
			case PROPERTY_CMD_1 :				
				if ( calibration( (int)input_value ) ) {
					DBG("acc_sensor_calibration OK\n");
					value = 0;
				} else {
					ERR("acc_sensor_calibration FAIL\n");
				}
				break;

			case PROPERTY_CMD_2 :
				return (long)m_calibration_flag;

				
			default :
				ERR("Invalid property_cmd\n");
				break;				
		}
	}
	else {
		ERR("Invalid sensor_type\n");		
	}
#else
	ERR("Cannot support any cmd\n");		
#endif

	return value;
	
}

int cgyro_sim::get_property(unsigned int property_level , void *property_data)
{
	DBG("gyro_sim_sensor called get_property , with property_level : 0x%x", property_level);
	if ( (property_level & 0xFFFF) == 1 ) {
	        base_property_struct *return_property;
	        return_property = (base_property_struct *)property_data;
	        return_property->sensor_unit_idx = IDX_UNIT_DEGREE_PER_SECOND;
		return_property->sensor_min_range = -(m_sensitivity*32768/1000);
          	return_property->sensor_max_range = (m_sensitivity*32767/1000);
                return_property->sensor_resolution = (m_sensitivity/1000);
		snprintf(return_property->sensor_name,   sizeof(return_property->sensor_name),   SENSOR_NAME  );
	        snprintf(return_property->sensor_vendor, sizeof(return_property->sensor_vendor), SENSOR_VENDOR);

        	return 0;
    	} else {
        	ERR("Doesnot support property_level : %d\n",property_level);
        	return -1;
    	}
}

int cgyro_sim::get_struct_value(unsigned int struct_type , void *struct_values)
{
#ifdef TARGET
	if ( (struct_type & 0xFFFF) == 0x0001) {
		base_data_struct *return_struct_value = NULL;
		return_struct_value = (base_data_struct *)struct_values;
		if ( return_struct_value ) {
			return_struct_value->data_accuracy = ACCURACY_NORMAL;
			return_struct_value->data_unit_idx = IDX_UNIT_DEGREE_PER_SECOND;
			return_struct_value->time_stamp = m_fired_time ;
			return_struct_value->values_num = 3;
			return_struct_value->values[0] = (m_x * m_sensitivity / 1000 );
			return_struct_value->values[1] = (m_y * m_sensitivity / 1000 );
			return_struct_value->values[2] = (m_z * m_sensitivity / 1000 );

			return 0;
		} else {
			ERR("return struct_value point error\n");
		}
		
	} else {
		ERR("Does not support type , struct_type : %d \n",struct_type);		
	} 
#endif
	return -1;
}

int cgyro_sim::check_hw_node(void)
{
#if 0
	char name_node[256];
	char hw_name[50];

	DIR *iio_main_dir = NULL;
	struct dirent *iio_dir_entry = NULL;

	FILE *fp;
	const char* orig_name = "K3G";
	int find_node = 0;

	struct stat lstat_buf;

	if (lstat("/dev/gyro_sim",&lstat_buf) != 0)
		DBG("Cannot find old style H/W node : /dev/gyro_sim for gyro_sim\n");
	else {
		if (S_ISCHR(lstat_buf.st_mode))
			return 1;
		else
			ERR("It's not char device node : /dev/gyro_sim (not real device for gyro_sim)\n");
	}

	iio_main_dir = opendir("/sys/bus/iio/devices/");
	if (!iio_main_dir) {
		ERR("iio-style dir open failed to collect data\n");
		return -1;
	}

	while ( (!find_node) && (iio_dir_entry = readdir(iio_main_dir)) ) {
		if ( (strncasecmp(iio_dir_entry->d_name ,".",1 ) != 0) && 
			(strncasecmp(iio_dir_entry->d_name ,"..",2 ) != 0) && 
			(iio_dir_entry->d_ino != 0) && (strlen(iio_dir_entry->d_name)<9) ) {
			snprintf(name_node,sizeof(name_node),"/sys/bus/iio/devices/%s/name",iio_dir_entry->d_name);
			fp = fopen(name_node, "r");

			if (!fp) {
				DBG("Failed to open a sys_node or there is no node: %s , so retry it\n",name_node);			
				continue;
			}
		
			if ( fscanf(fp, "%s", hw_name) < 0) {
				fclose(fp);
				DBG("Failed to collect data from %s , so retry it\n",name_node);
				continue;
			}
			fclose(fp);
	
			if ( (!strcasecmp(hw_name, orig_name )) ) {
				DBG("Find new style H/W  for gyro_sim(K3G)\n");
				find_node = 1;	
				break;
			}
		}
	}
	closedir(iio_main_dir);

	return find_node;
#else
	return 1;
#endif
}

int cgyro_sim::check_sensitivity(int update)
{
	int fd;
	int state;
	unsigned char fs_range = 0x10;	// L3G4200D_FS_500DPS
	
	fd = open(m_resource , O_RDWR);
	if ( fd < 0 )
		return -1;

	
	if (update) {
		m_sensitivity = L3G4200D_FS_500DPS_SENSITIVITY;
	/*	switch (fs_range ) {
			case L3G4200D_FS_250DPS:
				m_sensitivity = L3G4200D_FS_250DPS_SENSITIVITY;
				break;

			case L3G4200D_FS_500DPS:
				m_sensitivity = L3G4200D_FS_500DPS_SENSITIVITY;
				break;

			case L3G4200D_FS_2000DPS:
				m_sensitivity = L3G4200D_FS_2000DPS_SENSITIVITY;
				break;

			default:
				close(fd);
				return -1;
		}
	*/
	}

	DBG("Check sensitivity : 0x%x\n",fs_range);

	close(fd);
	return fs_range;	
}



cmodule *module_init(void *win, void *egl)
{
	cgyro_sim *sample;

	try {
		sample = new cgyro_sim();
	} catch (int ErrNo) {
		ERR("cgyro_sim class create fail , errno : %d , errstr : %s\n",ErrNo, strerror(ErrNo));
		return NULL;
	}

	return (cmodule*)sample;
}



void module_exit(cmodule *inst)
{
	cgyro_sim *sample = (cgyro_sim*)inst;
	delete sample;
}



//! End of a file
