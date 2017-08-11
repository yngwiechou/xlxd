//
//  cusb3003interface.cpp
//  ambed
//
//  Created by Jean-Luc Deltombe (LX3JL) on 23/04/2017.
//  Copyright © 2015 Jean-Luc Deltombe (LX3JL). All rights reserved.
//
// ----------------------------------------------------------------------------
//    This file is part of ambed.
//
//    xlxd is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    xlxd is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with Foobar.  If not, see <http://www.gnu.org/licenses/>.
// ----------------------------------------------------------------------------

#include "main.h"
#include "ctimepoint.h"
#include "cambepacket.h"
#include "cusb3000interface.h"
#include "cvocodecs.h"

////////////////////////////////////////////////////////////////////////////////////////
// configuration:
//
//      PKT_CHANNEL0 = DSTAR    (AMBEPLUS)
//      PKT_CHANNEL1 = DMR/C4FM (AMBE2PLUS)
//      PKT_CHANNEL2 = UNUSED or HYBRID


////////////////////////////////////////////////////////////////////////////////////////
// constructor

CUsb3000Interface::CUsb3000Interface(uint32 uiVid, uint32 uiPid, const char *szDeviceName, const char *szDeviceSerial)
: CUsb3xxxInterface(uiVid, uiPid, szDeviceName, szDeviceSerial)
{
    m_uiChCodecs[0]= CODEC_AMBEPLUS;
    m_uiChCodecs[1]= CODEC_AMBE2PLUS;
    m_uiChCodecs[2]= CODEC_NONE;
}

////////////////////////////////////////////////////////////////////////////////////////
// initialization

bool CUsb3000Interface::Init(uint8 uiOddCodec)
{
    bool ok = true;
    
    // init the odd channel
    m_uiChCodecs[2] = uiOddCodec;
    
    // base class
    ok &= CUsb3xxxInterface::Init();
    
    // do not create our channels now
    // this is delegated to caller (CVocodecs) as our channel
    // may be hybrids between 2 interfaces in case of odd n' of channel device)
    
    // done
    return ok;
}

////////////////////////////////////////////////////////////////////////////////////////
// manage Channels

uint8 CUsb3000Interface::GetChannelCodec(int iCh) const
{
    uint8 uiCodec = CODEC_NONE;
    if ( (iCh >= 0) && (iCh <= USB3000_NB_CH) )
    {
        uiCodec = m_uiChCodecs[iCh];
    }
    return uiCodec;
}

////////////////////////////////////////////////////////////////////////////////////////
// manage vocodec channels

CVocodecChannel *CUsb3000Interface::GetChannelWithChannelIn(int iCh)
{
    CVocodecChannel *Channel = NULL;
    bool done = false;
    for ( int i = 0; (i < m_Channels.size()) && !done; i++ )
    {
        if ( iCh == 2 )
        {
            if ( (m_Channels[i]->GetChannelIn() == iCh) && !(m_Channels[i]->IsInterfaceOut(this)) )
            {
                Channel = m_Channels[i];
                done = true;
            }
        }
        else
        {
            if ( (m_Channels[i]->GetChannelIn() == iCh) && (m_Channels[i]->IsInterfaceOut(this)) )
            {
                Channel = m_Channels[i];
                done = true;
            }
        }
    }
    return Channel;
}

CVocodecChannel *CUsb3000Interface::GetChannelWithChannelOut(int iCh)
{
    CVocodecChannel *Channel = NULL;
    bool done = false;
    for ( int i = 0; (i < m_Channels.size()) && !done; i++ )
    {
        if ( (m_Channels[i]->GetChannelOut() == iCh) && (m_Channels[i]->IsInterfaceOut(this)) )
        {
            Channel = m_Channels[i];
            done = true;
        }
    }
    return Channel;
}

////////////////////////////////////////////////////////////////////////////////////////
// low level

bool CUsb3000Interface::SoftResetDevice(void)
{
    bool ok = false;
    /*
    FT_STATUS ftStatus;
    int len;
    char rxpacket[100];
    
    //if the device is a USB-3003, it supports reset via UART break signal
    printf("reset via uart break...\n");
    ftStatus = FT_SetBreakOn( m_FtdiHandle );
    CTimePoint::TaskSleepFor(10);
    ftStatus = FT_SetBreakOff( m_FtdiHandle );
    CTimePoint::TaskSleepFor(10);

    len = FTDI_read_packet( m_FtdiHandle, rxpacket, sizeof(rxpacket) );
    ok = ((len == 7) && (rxpacket[4] == PKT_READY));
    if ( !ok )
    {
    	std::cout << "USB-3xxx soft reset failed" << std::endl;
    */
	
	//if the device is a USB-3000, do PKT_RESET instead
	std::cout << "Trying soft reset for USB-3000" << std::endl;
    
    DWORD n, b;
    char txpacket[10] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    char reset_packet[7] = { PKT_HEADER, 0, 3, 0, PKT_RESET, PKT_PARITYBYTE, 0 ^ 3 ^ 0 ^ PKT_RESET };
    char *p;
    FT_STATUS ftStatus;
    int len, i;
    char rxpacket[100];
	
    for (i = 0; i < 35; i++) {
		p = &txpacket[0];
		n = 10;
		do {
			ftStatus = FT_Write( m_FtdiHandle, p, n, &b);
			if (FT_OK != ftStatus) {
			 printf("force_soft_reset:1: Ft_Write error (ftStatus=%d)", ftStatus);
			 return 1;
			}
			n -= b;
			p += b;
		} while (n > 0);
    }
	
    p = &reset_packet[0];
    n = 7;
    do {
    	std::cout << "Sending PKT_RESET" << std::endl;
        ftStatus = FT_Write( m_FtdiHandle, p, n, &b);
        if (FT_OK != ftStatus) {
           printf("force_soft_reset:2: Ft_Write error (ftStatus=%d)", ftStatus);
           return 1;
        }
        n -= b;
        p += b;
    } while (n > 0);
	
    len = FTDI_read_packet( m_FtdiHandle, rxpacket, sizeof(rxpacket) );
    if (rxpacket[4] == PKT_READY)
    {
        printf("reset success\n");
        ok = true;
    }
    else
    {
    	printf("reset failure\n");
    }
          
    //}
    
    // done
    return ok;
}

bool CUsb3000Interface::ConfigureDevice(void)
{
    bool ok = true;
    uint8 pkt_ratep_ambeplus[]  = { 0x01,0x30,0x07,0x63,0x40,0x00,0x00,0x00,0x00,0x00,0x00,0x48 };
    uint8 pkt_ratep_ambe2plus[] = { 0x04,0x31,0x07,0x54,0x24,0x00,0x00,0x00,0x00,0x00,0x6F,0x48 };
    
    // configure each channels for desired codec
    for ( int i = 0; i < USB3000_NB_CH; i++ )
    {
        switch ( m_uiChCodecs[i] )
        {
            case CODEC_AMBEPLUS:
                ok &= ConfigureChannel(PKT_CHANNEL0+i, pkt_ratep_ambeplus, 0, 0);
                break;
            case CODEC_AMBE2PLUS:
                //ok &= ConfigureChannel(PKT_CHANNEL0+i, pkt_ratep_ambe2plus, -12, 0);   // DSTAR->DMR ok
                //ok &= ConfigureChannel(PKT_CHANNEL0+i, pkt_ratep_ambe2plus, 0, +10);   // DMR->DSTAR ok
                //ok &= ConfigureChannel(PKT_CHANNEL0+i, pkt_ratep_ambe2plus, -12, +10);   // not ok!
                ok &= ConfigureChannel(PKT_CHANNEL0+i, pkt_ratep_ambe2plus, 0, 0);
                break;
            case CODEC_NONE:
            default:
                break;
        }
    }

    // done
    return ok;
}

