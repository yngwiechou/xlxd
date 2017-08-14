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

CUsb3000Interface::CUsb3000Interface(uint32 uiVid, uint32 uiPid, const char *szDeviceName, const char *szDeviceSerial, int szCodec)
: CUsb3xxxInterface(uiVid, uiPid, szDeviceName, szDeviceSerial)
{
    m_uiChCodecs[0]= szCodec;
}

////////////////////////////////////////////////////////////////////////////////////////
// initialization

bool CUsb3000Interface::Init(uint8 uiOddCodec)
{
    bool ok = true;
    
    // init the odd channel
    //m_uiChCodecs[2] = uiOddCodec;
    // init ch0
    m_uiChCodecs[0] = uiOddCodec;
    
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
	
    // USB-3000 does not support reset via UART break signal
	// Do PKT_RESET instead
	std::cout << "Trying Packet Reset for USB-3000" << std::endl;
               
    int len;
	
    char wakeup_packet[10] =
    {
        0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00
    };
    // Must send with parity
    char reset_packet[7] =
    {
        PKT_HEADER,
        0,
        3,
        PKT_CONTROL,
        PKT_RESET,
        PKT_PARITYBYTE,
    	3 ^ PKT_CONTROL ^ PKT_RESET ^ PKT_PARITYBYTE
    };
    char rx_packet[100];
    
    printf ("[DEBUG] Sending 350 Wakeup packets\n");
    
    for (int i = 0; i < 35 ; i++)
    {
		FTDI_write_packet(m_FtdiHandle, wakeup_packet, sizeof(wakeup_packet));
	}
    
    printf ("[DEBUG] Sending PKT_RESET (with Parity)\n");
	
	if ( FTDI_write_packet(m_FtdiHandle, reset_packet, sizeof(reset_packet)))
	{
		len = FTDI_read_packet( m_FtdiHandle, rx_packet, sizeof(rx_packet) );
        ok = ( rx_packet[4] == PKT_READY );
	}
		
    if (ok)
    {
        printf("Soft Reset success\n");
    }
    else
    {
    	printf("Soft Reset failure\n");
    }
	    
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
            	std::cout << "Setting Channel: " << i << " Codec: " << CODEC_AMBEPLUS << std::endl;
                ok &= ConfigureChannel(PKT_CHANNEL0+i, pkt_ratep_ambeplus, 0, 0);
                break;
            case CODEC_AMBE2PLUS:
            	std::cout << "Configure Device Channel (" << i << ") with Codec: " << CODEC_AMBE2PLUS << std::endl;
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

