/*
 * main.c
 *
 *  Created on: 2013Äê9ÔÂ20ÈÕ
 *      Author: Administrator
 */


#include <cyg/infra/diag.h>
#include <cyg/kernel/kapi.h>


/* Application entry */
void cyg_user_start(void)
{

	diag_printf("test hello...\n");

	//cyg_scheduler_start();/* Scheduler start */

	while(1);
}
