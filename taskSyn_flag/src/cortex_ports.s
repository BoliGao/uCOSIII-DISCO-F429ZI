	.global PendSV_Handler
	.global SysTick_Handler

	.extern	OS_CPU_PendSVHandler
	.extern	OS_CPU_SysTickHandler

	.text
   	.align 2
   	.thumb
   	.syntax unified

.thumb_func
PendSV_Handler:
	b OS_CPU_PendSVHandler
	b .
	
.thumb_func	
SysTick_Handler:
	b OS_CPU_SysTickHandler
	b .
	
	.end
