/**
 * @file        main.c
 * @author      aSumo
 * @version     2.0
 * @date        2024-11-13
 * @copyright   (c) 2024 aSumo
 * @brief       A program for True-Bypass with PIC micro controller. 
 * @n           We call the module "Module01".
 * @details     verified: PIC12F675
 */



#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <xc.h>
#include "header.h"



/**
 * @brief 
 * 適用するモジュールの選択
 * 1: Latch     M01_forLatch.pdf
 * 2: UnLatch   M01_forUnLatch.pdf
 */
#define moduleNum 1
#define wait01 5    //!< PhotoCouplerの立ち上がりを待つ時間 [ms]
#define wait02 5    //!< RelayとLEDの動作を待つ時間 [ms]
#define wait03 10   //!< チャタリングが収まるのを待つ時間 [ms]

// wait01, wait02, wait03は適宜調整の必要あり



/**
 * @fn      void initialize(void)
 * @brief   ピンの初期化を行う関数
 * @details bypass関数を汎用するためにTRISIO3=FootSwitch, TRISIO4=ModeSwitchは死守すること
 */
void initialize(void) {
    ANSEL       = 0;            // アナログ入出力   不使用
    CMCON       = 0x07;         // コンパレータ     不使用
    ADCON0      = 0;            // AD/DAコンバータ  不使用
    OPTION_REG  = 0b00000010;   // プルアップ機能   使用
    WPU         = 0b00010000;   // GP4のみプルアップ

    switch(moduleNum) {
        case 1:
            TRISIO0 = 0;    // Relay_Set        出力
            TRISIO1 = 0;    // Relay_Reset      出力
            TRISIO2 = 0;    // PhotoCoupler     出力
            TRISIO3 = 1;    // FootSwitch       入力
            TRISIO4 = 1;    // ModeSwitch       入力（要プルアップ）
            TRISIO5 = 0;    // LED              出力
            GPIO    = 0;    // 全てのピンをLOWに設定

            // 念のためペダルを"OFF"にしておく
            GP1 = true;         // Relay_Set      ON
            __delay_ms(wait02); // 待つ
            GP1 = false;        // Relay_Set      OFF
            break;

        case 2:
            TRISIO0 = 0;    // Free             未割当
            TRISIO1 = 0;    // PhotoCoupler     出力
            TRISIO2 = 0;    // Relay            出力
            TRISIO3 = 1;    // FootSwitch       入力
            TRISIO4 = 1;    // ModeSwitch       入力（要プルアップ）
            TRISIO5 = 0;    // LED              出力
            GPIO    = 0;    // 全てのピンをLOWに設定
            break;
        
        default:
            break;
    }
}



/**
 * @fn              bool turn(bool)
 * @param State     the OLD state before this function executed
 * @return          the NEW state after this function executed
 * @brief           実際にペダルをON/OFFする関数
 * @details         Relay, LED and Photocoupler are controled.
 */
bool turn(bool State) {
    switch(moduleNum) {
        case 1:
            GP2 = true;         // PhotoCoupler ON
            __delay_ms(wait01); // PhotoCouplerの立ち上がりを待つ
            if (State) {        // "ON" → "OFF"
                GP1 = true;     // Relay_Reset  ON
                GP5 = false;    // LED          OFF
            } else {            // "OFF" → "ON"
                GP0 = true;     // Relay_Set    ON
                GP5 = true;     // LED          ON
            }
            __delay_ms(wait02); // RelayとLEDの動作を待つ
            GP0 = false;        // いずれにせよ着席
            GP1 = false;        // いずれにせよ着席
            GP2 = false;        // PhotoCoupler OFF
            break;

        case 2:
            GP1 = true;         // PhotoCoupler ON
            __delay_ms(wait01); // PhotoCouplerの立ち上がりを待つ
            GP2 = !State;       // Relay        turn
            GP5 = !State;       // LED          turn
            __delay_ms(wait02); // RelayとLEDの動作を待つ
            GP2 = false;        // PhotoCoupler OFF
            break;

        default:
            break;
    }

    State = !State;             // 状態を更新
    return State;
}



/**
 * @fn      void bypass(void)
 * @brief   バイパスの挙動を定義する関数
 * @details 今のところはモジュールによらず汎用
 */
void bypass(void) {
    bool state = false; // ペダルの状態変数（"ON": true, "OFF": false）
    int pressed = 0;    // 踏まれていたカウント
    int released = 2;   // 離されていたカウント

    // Mugen Loop
    while(true) {
        // Was pressed or released?
        if(GP3) {                           // released (not pressed)
            __delay_ms(wait03);             // wait for chattering...
            if(GP3) {                       // surely released
                pressed = 0;                // reset "pressed"
                released++;                 // count up "released"
                if(released==1 && !GP4) {   // Is this your first time and in the Momentary mode?
                    state = turn(state);    // Yes: Do "turn"!
                } else {                    // No:
                    released = 2;           // set "released" 2 (MAX)
                }
            }
        } else {                            // pressed
            __delay_ms(wait03);             // wait for chattering...
            if(!GP3) {                      // surely pressed
                released = 0;               // reset "released"
                pressed++;                  // count up "pressed"
                if(pressed == 1) {          // Is this your first time?
                    state = turn(state);    // Yes: Do "turn"!
                } else {                    // No:
                    pressed = 2;            // set "pressed" 2 (MAX)
                }
            }
        }
    }
}



/**
 * @fn      void main(void)
 * @brief   サボり上司
 */
void main(void) {
    initialize();   // ピンの初期化
    bypass();       // ひたすらループ
}
