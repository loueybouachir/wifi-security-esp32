/*
 * deauth_attack.h
 *
 *  Created on: 25 mai 2025
 *      Author: HP
 */

#ifndef MAIN_DEAUTH_ATTACK_H_
#define MAIN_DEAUTH_ATTACK_H_

void sendDeauthFrame(uint8_t bssid[6], int channel, uint8_t mac[6]);

void start_deauth_attack();

#endif /* MAIN_DEAUTH_ATTACK_H_ */
