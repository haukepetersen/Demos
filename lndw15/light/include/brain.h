/*
 * Copyright (C) 2015 Hauke Petersen <mail@haukepetersen.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

/**
 * @ingroup     scala
 * @{
 *
 * @file
 * @brief       Interfaces of Scala's brain
 *
 * @author      Hauke Petersen <mail@haukepetersen.de>
 * @author      Cenk Gündoğan <cnkgndgn@gmail.com>
 */

#ifndef BRAIN_H_
#define BRAIN_H_

#ifdef __cplusplus
 extern "C" {
#endif

/**
 * @brief   Initialize Scala and start the brain's thread
 */
void brain_init(void);

/**
 * @brief   Set the level of the scala
 *
 * @param[in] level     level of the scala
 */
void brain_scala_level(uint8_t level);

/**
 * @brief   Set the raw level of the scala
 *
 * @param[in] level     level of the scala
 */
void brain_scala_level_raw(int level);

#ifdef __cplusplus
}
#endif

#endif /* BRAIN_H_*/
/** @} */
