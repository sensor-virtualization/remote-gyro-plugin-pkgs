/*
 * emulator-plugin-gyro-pkgs
 *
 * Copyright (c) 2000 - 2011 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact:
 * SooYoung Ha <yoosah.ha@samsnung.com>
 * Sungmin Ha <sungmin82.ha@samsung.com>
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
 */

class gyro_sim_processor : public cprocessor_module
{
public:
	enum gyro_sim_data_id {
		GYRO_BASE_DATA_SET = (0x0020<<16) | 0x0001,
	};

	enum gyro_sim_evet_type_t {
		GYROSCOPE_EVENT_RAW_DATA_REPORT_ON_TIME			= (0x0020<<16) |0x0001,
	};

	enum gyro_sim_cmd_property_t {
		PROPERTY_CMD_START = 0,
		PROPERTY_CMD_1,
		PROPERTY_CMD_2,
		PROPERTY_CMD_3,
		PROPERTY_CMD_4,
		PROPERTY_CMD_5,
	};

	gyro_sim_processor();
	virtual ~gyro_sim_processor();

	const char *name(void);
	int id(void);
	int version(void);

	bool update_name(char *name);
	bool update_id(int id);
	bool update_version(int version);

	bool add_input(csensor_module *sensor);
	bool add_input(cfilter_module *filter);

	long value(char *port);
	long value(int id);
	//float value(char *port);
	//float value(int id);

	cprocessor_module *create_new(void);
	void destroy(cprocessor_module *module);

	static void *working(void *inst);
	static void *stopped(void *inst);

	virtual bool start(void);
	virtual bool stop(void);

	bool add_callback_func(cmd_reg_t *param);
	bool remove_callback_func(cmd_reg_t *param);
	bool check_callback_event(cmd_reg_t *param);
	
	long set_cmd(int type , int property , long input_value);
	int get_property(unsigned int property_level , void *property_data );
	int get_struct_value(unsigned int struct_type , void *struct_values);	
	
	int check_hw_node(void);


private:
	csensor_module *m_sensor;
	cfilter_module *m_filter;

	long m_x;
	long m_y;
	long m_z;

	//float m_x;
	//float m_y;
	//float m_z;

	long m_event;
	long m_new_event;

	long m_version;
	long m_id;

	char *m_name;

	csync m_sync;
	
	int m_client;
	int m_work_err_count;
			
	unsigned int m_data_report_cb_client;	
	
};



//! End of a file
