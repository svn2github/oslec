#!/bin/sh
# ./oslec-ctrl-panel.sh
#
# David Rowe
# Created February 2007
#
# cdialog based shell script to control Oslec echo canceller in real time
# from a text mode GUI.  I am totally useless at shell scripting so please
# feel free to improve on this.

#
#  Copyright (C) 2007 David Rowe
# 
#  All rights reserved.
# 
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License version 2, as
#  published by the Free Software Foundation.
# 
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
# 
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

nlp_off=off
nlp_mute=off
nlp_cng=off
nlp_clip=on
nlp_mode=1+2+8

tmpfile=`mktemp /tmp/oslec_panel_XXXXXX` || exit 1

nlp_menu()
{
  nlpstatus=0
  while [ $nlpstatus -eq 0 ]
  do
  
    dialog \
      --radiolist "NLP Mode" 10 70 4 \
          Off  "NLP disabled" $nlp_off  \
          Mute "Simple mute" $nlp_mute  \
          CNG  "Synthetic level-matched comfort noise" $nlp_cng \
          Clip "Clip to background noise level" $nlp_clip \
        2>$tmpfile 
    nlpstatus=$?
 
    if [ $nlpstatus -eq 0 ] ; then
      nlpmenuitem=`cat $tmpfile` 
      case $nlpmenuitem in
	  "Off") nlp_off=on; nlp_mute=off; nlp_cng=off; nlp_clip=off; nlp_mode=0;;
	  "Mute") nlp_off=off; nlp_mute=on; nlp_cng=off; nlp_clip=off; nlp_mode=2;;
	  "CNG") nlp_off=off; nlp_mute=off; nlp_cng=on; nlp_clip=off; nlp_mode=`expr 2 + 4`;;
	  "Clip") nlp_off=off; nlp_mute=off; nlp_cng=off; nlp_clip=on; nlp_mode=`expr 2 + 8`;;
      esac
      mode=`expr 1 + $nlp_mode`
      echo $mode > /proc/oslec/mode
    fi
  done

  return
}

filter_menu()
{
    
  filterstatus=0
  while [ $filterstatus -eq 0 ]
  do
  
    # extract HPF state from $mode
    mode=`cat /proc/oslec/mode`
    tx_hpf=$(( $mode & 16 ))
    if [ $tx_hpf -eq 16 ] ; then
        tx_hpf=on
    else
        tx_hpf=off
    fi
    rx_hpf=$(( $mode & 32 ))
    if [ $rx_hpf -eq 32 ] ; then
      rx_hpf=on
    else
      rx_hpf=off
    fi

    dialog \
      --checklist "High Pass Filters $mode" 10 70 4 \
          Tx  "Tx HPF" $tx_hpf  \
          Rx  "Rx HPF" $rx_hpf  \
        2>$tmpfile 
    filterstatus=$?
 
    if [ $filterstatus -eq 0 ] ; then
      filtermenuitem=`cat $tmpfile` 
      if [ `expr match "$filtermenuitem" ".*Tx"` -ne 0 ] ; then
	  mode=$(( $mode | 16 ))
      else

	  mode=$(( $mode & 239 ))
      fi
      
      if [ `expr match "$filtermenuitem" ".*Rx"` -ne 0 ] ; then
	  mode=$(( $mode | 32 ))
      else
	  mode=$(( $mode & 223 ))
      fi
      echo $mode > /proc/oslec/mode
    fi
  done

  return
}

disable() {
    mode=`cat /proc/oslec/mode`
    mode=$(( $mode | 64 ))
    echo $mode > /proc/oslec/mode
}

enable() {
    mode=`cat /proc/oslec/mode`
    mode=$(( $mode & 191 ))
    echo $mode > /proc/oslec/mode
}

status=0
menuitem=Info
while [ $status -eq 0 ] 
do
  dialog  --cancel-label Finish --default-item $menuitem \
    --menu "Oslec Control Panel" 12 60 6 \
    Info      "Display echo canceller stats" \
    NLP       "Configure Non Linear Processor" \
    Filters   "Select High Pass Filters" \
    Reset     "Reset echo canceller" \
    Disable   "Disable echo canceller" \
    Enable    "Enable echo canceller" \
    2>$tmpfile 

  status=$?
 
  if [ $status -eq 0 ] ; then
    menuitem=`cat $tmpfile` 
    case $menuitem in
	  "Info") cat /proc/oslec/info>$tmpfile; dialog --textbox $tmpfile 18 60;;
	  "NLP") nlp_menu;;
	  "Filters") filter_menu;;
	  "Reset")   echo 1 > /proc/oslec/reset;;
	  "Disable") disable;;
	  "Enable") enable;;
      esac
    fi
done

rm $tmpfile
