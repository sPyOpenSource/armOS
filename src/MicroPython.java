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

public class MicroPython extends JVSMain
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

        initPins(5, 5, 5, 5);
        setSize(100, 100);
        element.jSetPinDescription(0, "GPIO0");
        element.jSetPinDescription(2, "GPIO2");
        element.jSetPinDescription(3, "GPIO3");
        element.jSetPinDescription(4, "GPIO4");


        element.jSetPinDescription(5, "GPIO4");
        element.jSetPinDescription(6, "GPIO6");
        element.jSetPinDescription(7, "GPIO7");
        element.jSetPinDescription(8, "GPIO8");
        element.jSetPinDescription(9, "GPIO9");
    }
}
