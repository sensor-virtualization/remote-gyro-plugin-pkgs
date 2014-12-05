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

class cgyro_sim : public csensor_module
{
public:
	enum gyro_sim_cmd_property_t {
		PROPERTY_CMD_START = 0,
		PROPERTY_CMD_1,
		PROPERTY_CMD_2,
		PROPERTY_CMD_3,
		PROPERTY_CMD_4,
		PROPERTY_CMD_5,
	};

	cgyro_sim();
	virtual ~cgyro_sim();

	const char *name(void);
	int version(void);
	int id(void);

	bool is_data_ready(bool wait=false);

	long value(const char *port);
	long value(int id);

	bool update_name(char *name);
	bool update_version(int ver);
	bool update_id(int id);

	int port_count(void);
	const char *port(int idx);

	bool need_polling(void);
	long polling_interval(void);
	bool update_polling_interval(unsigned long val);
	int get_sensor_type(void);
	long set_cmd(int type , int property , long input_value);
	int get_property(unsigned int property_level , void *property_data);
	int get_struct_value(unsigned int struct_type , void *struct_values);	

	
	int check_hw_node(void);
	int check_sensitivity(int update);
	

	bool start(void);
	bool stop(void);

	void reset(void);	
	
private:

	static const char *m_port[];

	char *m_name;
	char *m_resource;
	long m_id;
	long m_version;
	unsigned long m_polling_interval;

	int m_x;	//pitch
	int m_y;	//roll	
	int m_z;	//yaw

	unsigned long long m_fired_time;

	bool update_value(void);

	int m_client;
	int m_sensor_type;

	int m_ioctl_fd;
	float m_sensitivity;
	bool m_hw_type_k3g;

};



//! End of a file
