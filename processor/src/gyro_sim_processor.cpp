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
#include <netinet/in.h>
#include <math.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/input.h>
#include <dirent.h>

#include <common.h>
#include <cobject_type.h>
#include <clist.h>
#include <cmutex.h>
#include <cmodule.h>
#include <csync.h>
#include <cworker.h>
#include <cpacket.h>
#include <csock.h>
#include <sf_common.h>

#include <csensor_module.h>
#include <cfilter_module.h>
#include <cprocessor_module.h>
#include <gyro_sim_processor.h>

#include <vconf.h>


gyro_sim_processor::gyro_sim_processor()
: m_sensor(NULL)
, m_filter(NULL)
, m_x(-1)
, m_y(-1)
, m_z(-1)
, m_event(0)
, m_new_event(0)
, m_version(1)
, m_id(0x04BE)
, m_client(0)
, m_work_err_count(0)
, m_data_report_cb_client(0)
{
	m_name = strdup("gyro_processor");
	if ((!m_name)) {
		free(m_name);		
		throw ENOMEM;
	}

#ifdef HWREV_CHECK
//	if (check_hw_node() != 1 ) {
	//	free(m_name);
		//throw ENXIO;
	//}
#endif

	cprocessor_module::set_main(working, stopped, this);
}



gyro_sim_processor::~gyro_sim_processor()
{
	free(m_name);
	
}



bool gyro_sim_processor::add_input(csensor_module *sensor)
{
	m_sensor = sensor;
	return true;
}



bool gyro_sim_processor::add_input(cfilter_module *filter)
{
	m_filter = filter;
	return true;
}



const char *gyro_sim_processor::name(void)
{
	return m_name;
}



int gyro_sim_processor::id(void)
{
	return m_id;
}



int gyro_sim_processor::version(void)
{
	return m_version;
}



bool gyro_sim_processor::update_name(char *name)
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



bool gyro_sim_processor::update_id(int id)
{
	m_id = id;
	return true;
}



bool gyro_sim_processor::update_version(int version)
{
	m_version = version;
	return true;
}



cprocessor_module *gyro_sim_processor::create_new(void)
{
#ifdef USE_ONLY_ONE_MODULE
	//return (cprocessor_module*)this;
#else
	gyro_sim_processor *inst = NULL;
	bool bstate = false;

	try {
		inst = new gyro_sim_processor;
	} catch (...) {
		ERR("No Memory\n");
		return NULL;
	}

	bstate = cmodule::add_to_list((cmodule *)inst);
	if ( !bstate ) {
		ERR("Creat and add_to_list fail");
		return NULL;
	}	

	return (cprocessor_module*)inst;
#endif
}



void gyro_sim_processor::destroy(cprocessor_module *module)
{
	bool bstate = false;

	bstate = cmodule::del_from_list((cmodule *)module);

	if ( !bstate ) {
		ERR("Destory and del_from_list fail");
		delete (gyro_sim_processor *)module;
		return ;
	}	
}



void *gyro_sim_processor::stopped(void *inst)
{

	gyro_sim_processor *processor = (gyro_sim_processor*)inst;

	if (!processor) {
		ERR("There is no processor module instance at gyro_sim (%s)\n", __FUNCTION__ );
		return (void*)NULL;
	}

	processor->wakeup_all_client();

	DBG(">>>>>>>>Wait signal %lx , %s\n", pthread_self(), processor->m_name);
	processor->m_sync.wait();

	DBG(">>>>>>>>>Signal received %lx, %s\n", pthread_self(), processor->m_name);
	return (void*)NULL;
}



void *gyro_sim_processor::working(void *inst)
{	
	csensor_module *sensor;

	gyro_sim_processor *processor = (gyro_sim_processor*)inst;
	if (!processor) {
		ERR("There is no processor module instance at gyro_sim (%s)\n", __FUNCTION__ );
		return (void*)cworker::STOPPED;
	}

#ifdef TARGET
	DBG("Gathering data\n");
	if (!processor->m_sensor) {
		ERR("Sensor is not added\n");
		return (void*)cworker::STOPPED;
	}

	//! Implementation dependent
	sensor = (csensor_module*)processor->m_sensor;

	DBG("Invoke is_data_ready\n");
	
	if (sensor->is_data_ready(true) == false) {
		ERR("Data ready has failed\n");
		processor->m_work_err_count++;
//		if ( processor->m_work_err_count > 10 ) {
//			ERR("Too many error counted stop processor");			
//			return (void*)cworker::STOPPED;
//		}
		return (void*)cworker::STARTED;		
	}
	
	processor->m_x = sensor->value(0);
	processor->m_y = sensor->value(1);
	processor->m_z = sensor->value(2);
	
	
	DBG("Data is ready now\n");
#else
	usleep(100000);
	
#endif

	processor->wakeup_all_client();

	//! TODO: How can I get the polling interval?
	//! TODO: When we get a polling interval, try read data in that interval :D
	return (void*)cworker::STARTED;
}



long gyro_sim_processor::value(char *port)
{
	if (!strcasecmp(port, "pitch")) {
		return m_x;
	} else if (!strcasecmp(port, "roll")) {
		return m_y;
	} else if (!strcasecmp(port, "yaw")) {
		return m_z;
	}
	
	return -1;
}



long gyro_sim_processor::value(int id)
{
	
	return -1;
}



bool gyro_sim_processor::start(void)
{
	bool ret;

	m_client ++;
	if (m_client > 1) {
		DBG("%s processor fake starting\n",m_name);
		return true;
	}

	DBG("%s processor real starting\n",m_name);

	//! Before starting the processor module,
	//! We have to enable sensor
	ret = m_sensor ? m_sensor->start() : true;
	if ( ret != true ) {
		ERR("m_sensor start fail\n");
		return false;
	}

	ret = m_filter ? m_filter->start() : true;
	if ( ret != true ) {
		ERR("m_filter start fail\n");
		return false;
	}

#ifdef TARGET
	ret = cprocessor_module::start();

	if (ret == true) {
		DBG("Signal send %s\n", m_name);
		m_sync.send_event();
		DBG("Signal sent\n");		
	}
#endif

	return ret;
}



bool gyro_sim_processor::stop(void)
{
	bool ret;

	m_client --;
	if (m_client > 0) {
		DBG("%s processor fake Stopping\n",m_name);
		return true;
	}

	DBG("%s processor real Stopping\n",m_name);
	
	m_client = 0;

#ifdef TARGET
	ret = cprocessor_module::stop();
	if ( ret != true ) {
		ERR("cprocessor_module::stop()\n");
		return false;
	}
#endif

	ret = m_filter ? m_filter->stop() : true;
	if ( ret != true ) {
		ERR("m_filter stop fail\n");
		return false;
	}

	ret = m_sensor ? m_sensor->stop() : true;
	if ( ret != true ) {
		ERR("m_sensor stop fail\n");
		return false;
	}

	
	return ret;
}

bool gyro_sim_processor::add_callback_func(cmd_reg_t * param)
{
//	char dummy_key[MAX_KEY_LEN];
	
	if ( param->type != REG_ADD ) {
		ERR("invaild cmd type !!");
		return false;
	}
	
	switch ( param->event_type ) {
		case GYROSCOPE_EVENT_RAW_DATA_REPORT_ON_TIME:
			m_data_report_cb_client++;
			break;
		default:
			ERR("invaild event type !!");
			return false;
	}

	return true;
}


bool gyro_sim_processor::remove_callback_func(cmd_reg_t * param)
{
	if ( param->type != REG_DEL ) {
		ERR("invaild cmd type !!");
		return false;
	}

	switch ( param->event_type ) {
		case GYROSCOPE_EVENT_RAW_DATA_REPORT_ON_TIME:
			m_data_report_cb_client--;
			break;
		default:
			ERR("invaild event type !!");
			return false;
	}	

	return true;
}

bool gyro_sim_processor::check_callback_event(cmd_reg_t *param)
{
	if ( param->type != REG_CHK ) {
		ERR("invaild cmd type !!");
		return false;
	}

	switch ( param->event_type ) {
		case GYROSCOPE_EVENT_RAW_DATA_REPORT_ON_TIME:			 
			 DBG("event check ok\n");
			break;
			
		default:
			ERR("invaild event type !!");
			return false;
	}	

	return true;
}

long gyro_sim_processor::set_cmd(int type , int property , long input_value)
{
	return -1;
}

int gyro_sim_processor::get_property(unsigned int property_level , void *property_data )
{
	DBG("gyro_sim_processor called get_property , with property_level : 0x%x", property_level);
	if (m_sensor) {
		return m_sensor->get_property(property_level , property_data);		
	} else {
		ERR("no m_sensor , cannot get_struct_value from sensor\n");
		return -1;
	}
}

int gyro_sim_processor::get_struct_value(unsigned int struct_type , void *struct_values)
{
	if (m_sensor) {
		if ( struct_type == GYRO_BASE_DATA_SET  ) {
			return m_sensor->get_struct_value(struct_type , struct_values);
		} else {
			ERR("does not support stuct_type\n");
			return -1;
		}
	} else {
		ERR("no m_sensor , cannot get_struct_value from sensor\n");
		return -1;
	} 
}

int gyro_sim_processor::check_hw_node(void)
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


cmodule *module_init(void *win, void *egl)
{
	gyro_sim_processor *inst;

	try {
		inst = new gyro_sim_processor();
	} catch (int ErrNo) {
		ERR("gyro_sim_processor class create fail , errno : %d , errstr : %s\n",ErrNo, strerror(ErrNo));
		return NULL;
	}

	return (cmodule*)inst;
}



void module_exit(cmodule *inst)
{
	gyro_sim_processor *sample = (gyro_sim_processor*)inst;
	delete sample;
}



//! End of a file
