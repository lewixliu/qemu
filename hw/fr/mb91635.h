/*
 * MB91635 Simulator.
 *
 * Copyright (c) 2022 Lewis Liu <lewix@ustc.edu>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see
 * <http://www.gnu.org/licenses/lgpl-2.1.html>
 */

#ifndef _MB91635_H_
#define _MB91635_H_

#define IOSIZE 0x1000

#define REGISTER_DICR 0x00000044 //Delayed interrupt control register

#define REGISTER_TMRLRA0 0x00000048 //16-bit timer reload register A0
#define REGISTER_TMR0 0x0000004A //16-bit timer register 0
#define REGISTER_TMCSR0 0x0000004E //Timer control status register 0
#define REGISTER_TMRLRA1 0x00000050 //16-bit timer reload register A1
#define REGISTER_TMR1 0x00000052 //16-bit timer register 1
#define REGISTER_TMCSR1 0x00000056 //Timer control status register 1
#define REGISTER_TMRLRA2 0x00000058 //16-bit timer reload register A2
#define REGISTER_TMR2 0x0000005A //16-bit timer register 2
#define REGISTER_TMCSR2 0x0000005E //Timer control status register 2

#define REGISTER_ICR00 0x00000440 //Interrupt control register 00
#define REGISTER_ICR47 0x0000046F //Interrupt control register 47

#define REGISTER_CSELR 0x00000510 //Clock source select register
#define REGISTER_CMONR 0x00000511 //Clock source monitor register

#endif