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
#define moduleNum   1

// 適宜調整の必要あり
#define wait01 5    //!< PhotoCouplerの立ち上がりを待つ時間 [ms]
#define wait02 5    //!< RelayとLEDの動作を待つ時間 [ms]
#define wait03 10   //!< チャタリングが収まるのを待つ時間 [ms]



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
            #define Relay_Set       GP0 //!< Relay_Setピン
            #define Relay_Reset     GP1 //!< Relay_Resetピン
            #define PhotoCoupler    GP2 //!< PhotoCouplerピン
            #define FootSwitch      GP3 //!< FootSwitchピン
            #define ModeSwitch      GP4 //!< ModeSwitchピン
            #define LED             GP5 //!< LEDピン
            TRISIO0 = 0;    // Relay_Set        出力
            TRISIO1 = 0;    // Relay_Reset      出力
            TRISIO2 = 0;    // PhotoCoupler     出力
            TRISIO3 = 1;    // FootSwitch       入力
            TRISIO4 = 1;    // ModeSwitch       入力（要プルアップ）
            TRISIO5 = 0;    // LED              出力
            GPIO    = 0;    // 全てのピンをLOWに設定

            // 念のためペダルを"OFF"にしておく
            Relay_Reset = true;     // Relay_Set      ON
            __delay_ms(wait02);     // 待つ
            Relay_Reset = false;    // Relay_Set      OFF
            break;

        case 2:
            #define Free            GP0 //!< Relayピン
            #define PhotoCoupler    GP1 //!< PhotoCouplerピン
            #define Relay           GP2 //!< Relayピン
            #define FootSwitch      GP3 //!< FootSwitchピン
            #define ModeSwitch      GP4 //!< ModeSwitchピン
            #define LED             GP5 //!< LEDピン
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
 * @details         Relay, LED and PhotoCoupler are controled.
 */
bool turn(bool State) {
    switch(moduleNum) {
        case 1:
            PhotoCoupler = true;    // PhotoCoupler ON
            __delay_ms(wait01);     // PhotoCouplerの立ち上がりを待つ
            if (State) {            // "ON" → "OFF"
                Relay_Reset = true; // Relay_Reset  ON
                LED = false;        // LED          OFF
            } else {                // "OFF" → "ON"
                Relay_Set = true;   // Relay_Set    ON
                LED = true;         // LED          ON
            }
            __delay_ms(wait02);     // RelayとLEDの動作を待つ
            Relay_Set = false;      // いずれにせよ着席
            Relay_Reset = false;    // いずれにせよ着席
            PhotoCoupler = false;   // PhotoCoupler OFF
            break;

        case 2:
            PhotoCoupler = true;    // PhotoCoupler ON
            __delay_ms(wait01);     // PhotoCouplerの立ち上がりを待つ
            Relay = !State;         // Relay        turn
            LED = !State;           // LED          turn
            __delay_ms(wait02);     // RelayとLEDの動作を待つ
            PhotoCoupler = false;   // PhotoCoupler OFF
            break;

        default:
            break;
    }

    State = !State; // 状態を更新
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
        if(FootSwitch) {                    // released (not pressed)
            __delay_ms(wait03);             // wait for chattering...
            if(FootSwitch) {                // surely released
                pressed = 0;                // reset "pressed"
                released++;                 // count up "released"
                // Is this your first time and in the Momentary mode?
                if(released==1 && !ModeSwitch) {
                    state = turn(state);    // Yes: Do "turn"!
                } else {                    // No:
                    released = 2;           // set "released" 2 (MAX)
                }
            }
        } else {                            // pressed
            __delay_ms(wait03);             // wait for chattering...
            if(!FootSwitch) {               // surely pressed
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
