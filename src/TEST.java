//*****************************************************************************
//* Element of MyOpenLab Library                                              *
//*                                                                           *
//* Copyright (C) 2004  Carmelo Salafia (cswi@gmx.de)                         *
//*                                                                           *
//* This library is free software; you can redistribute it and/or modify      *
//* it under the terms of the GNU Lesser General Public License as published  *
//* by the Free Software Foundation; either version 2.1 of the License,       *
//* or (at your option) any later version.                                    *
//* http://www.gnu.org/licenses/lgpl.html                                     *
//*                                                                           *
//* This library is distributed in the hope that it will be useful,           *
//* but WITHOUTANY WARRANTY; without even the implied warranty of             *
//* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                      *
//* See the GNU Lesser General Public License for more details.               *
//*                                                                           *
//* You should have received a copy of the GNU Lesser General Public License  *
//* along with this library; if not, write to the Free Software Foundation,   *
//* Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110, USA                  *
//*****************************************************************************

import VisualLogic.*;
import VisualLogic.variables.VSComboBox;
import java.util.ArrayList;
import tools.*;

public class TEST extends JVSMain
{
  private MyOpenLabDriverIF driver;
  private final VSComboBox comport = new VSComboBox();
  
  @Override
  public void init()
  {
      ArrayList<Object> args = new ArrayList<>();
        args.add("NULL");
        args.add(0);
        args.add(0);
        args.add(0);
        args.add(0);
        args.add(false);
    driver = element.jOpenDriver("MyOpenLab.RS232", args);
        
    System.out.println("driver=" + driver.toString());
    
    comport.clear();
    comport.addItem("NULL");
        
    if (driver != null) {
          ArrayList<String> ports = new ArrayList();
          driver.sendCommand("NULL;GETPORTS", ports);

          System.out.println("size of Ports : " + ports.size());
          ports.stream().map((port) -> {
              System.out.println("-X-------> Port : " + port);
            return port;
        }).forEachOrdered((port) -> {
            comport.addItem(port);
        });

          driver.sendCommand("NULL;CLOSE", null);
          element.jCloseDriver("MyOpenLab.RS232");
    } else {
          System.out.println("DRIVER IS NULL!!!");
    }
    
    initPins(5,5,5,5);
    setSize(100,100);
    element.jSetPinDescription(0,"AAAAAA");
    element.jSetPinDescription(1,"BBBBB");
    element.jSetPinDescription(2,"CCCC");
    element.jSetPinDescription(3,"DD");
    element.jSetPinDescription(4,"E");
    
    
    element.jSetPinDescription(5,"FFF");
    element.jSetPinDescription(6,"GGG");
    element.jSetPinDescription(7,"HHH");
    element.jSetPinDescription(8,"III");
    element.jSetPinDescription(9,"JJJ");
    
    element.jSetPinDescription(10,"KKK");
    element.jSetPinDescription(11,"LLL");
    element.jSetPinDescription(12,"MMM");
    element.jSetPinDescription(13,"NNN");
    element.jSetPinDescription(14,"OOO");
    
    element.jSetPinDescription(15,"PPP");
    element.jSetPinDescription(16,"QQQ");
    element.jSetPinDescription(17,"RRR");
    element.jSetPinDescription(18,"SSS");
    element.jSetPinDescription(19,"TTT");
  }

}
