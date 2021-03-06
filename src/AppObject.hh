/*
 * Copyright 2015 Applied Research Center for Computer Networks
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <string>
#include <time.h>

#include "JsonParser.hh"

/**
* This abstract class is used in applications.
* Objects in your app must inherit this class if app uses event model
*/
class AppObject
{
    /// When object was created
    time_t since;
public:
    AppObject(): since(0) {}

    /**
     * You must define JSON representation for your object
     */
    virtual JSONparser formJSON() = 0;

    /**
     * 64-bit unique identifier for your object
     */
    virtual uint64_t id() const = 0;

    /**
     * Object's created time getter and setter
     */
    time_t connectedSince();
    void connectedSince(time_t time);

    /**
     * Translate 64-bit identifier (DPID in switches) to string format
     */
    static std::string uint64_to_string(uint64_t dpid);

    /**
     * Define the equality operator between objects
     */
    friend bool operator==(const AppObject& o1, const AppObject& o2);
};
