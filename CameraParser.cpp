/*
 * Copyright (C) Texas Instruments - http://www.ti.com/
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
* @file CameraParser.cpp
*
* This file re-implements the TICameraCameraProperties.xml XML parser to load camera settings into ICS
*
*/

#include <libxml/parser.h>
#include <libxml/xmlreader.h>
#include <libxml/xmlwriter.h>
#include <libxml/tree.h>

#undef TRUE
#undef FALSE

#include "CameraHal.h"
#include "DebugUtils.h"
#include "CameraProperties.h"

#define CAMERA_ROOT         "CameraRoot"
#define CAMERA_INSTANCE     "CameraInstance"
#define TEXT_XML_ELEMENT    "#text"

namespace android {

const char CameraProperties::TICAMERA_FILE_PREFIX[] = "TICamera";
const char CameraProperties::TICAMERA_FILE_EXTN[] = ".xml";

///Loads the properties XML files present inside /system/etc and loads all the Camera related properties
status_t CameraProperties::loadXMLProperties() {
    LOG_FUNCTION_NAME
    status_t ret = NO_ERROR;

    // Open /system/etc directory and read all the xml file with prefix as TICamera<CameraName>
    DIR *dirp;
    struct dirent *dp;

    CAMHAL_LOGDA("Opening /system/etc directory");
    if ((dirp = opendir("/system/etc")) == NULL) {
        CAMHAL_LOGEA("Couldn't open directory /system/etc");
        LOG_FUNCTION_NAME_EXIT
        return UNKNOWN_ERROR;
    }

    CAMHAL_LOGDA("Opened /system/etc directory successfully");

    CAMHAL_LOGDA("Processing all directory entries to find Camera property files");
    do {
        errno = 0;
        if ((dp = readdir(dirp)) != NULL) {
            CAMHAL_LOGDB("File name %s", dp->d_name);
            char* found = strstr(dp->d_name, TICAMERA_FILE_PREFIX);
            if ((int32_t)found == (int32_t)dp->d_name) {
                // Prefix matches
                // Now check if it is an XML file
                if (strstr(dp->d_name, TICAMERA_FILE_EXTN)) {
                    // Confirmed it is an XML file, now open it and parse it
                    CAMHAL_LOGDB("Found Camera Properties file %s", dp->d_name);
                    snprintf(mXMLFullPath, sizeof(mXMLFullPath)-1, "/system/etc/%s", dp->d_name);
                    ret = parseAndLoadProps( ( const char * ) mXMLFullPath);
                    if ( ret != NO_ERROR ) {
                        (void) closedir(dirp);
                        CAMHAL_LOGEB("Error when parsing the config file :%s Err[%d]", mXMLFullPath, ret);
                        LOG_FUNCTION_NAME_EXIT
                        return ret;
                    }
                    CAMHAL_LOGDA("Parsed configuration file and loaded properties");
                }
            }
        }
    } while (dp != NULL);

    CAMHAL_LOGVA("Closing the directory handle");
    (void) closedir(dirp);

    LOG_FUNCTION_NAME_EXIT
    return ret;
}


status_t CameraProperties::parseAndLoadProps(const char* file) {
    LOG_FUNCTION_NAME

    xmlTextReaderPtr reader;
    const xmlChar *name=NULL, *value = NULL;
    status_t ret = NO_ERROR;
    char val[256];
    const xmlChar *nextName = NULL;
    int curCameraIndex;
    int propertyIndex;

    reader = xmlNewTextReaderFilename(file);
    if (reader != NULL) {
        ret = xmlTextReaderRead(reader);
        if (ret != 1) {
            CAMHAL_LOGEB("%s : failed to parse\n", file);
        }
        else {

            // xmlNode passed to this function is a cameraElement
            // It's child nodes are the properties
            // XML structure looks like this
            /**<CameraRoot>
              * <CameraInstance>
              * <CameraProperty1>Value</CameraProperty1>
              * ....
              * <CameraPropertyN>Value</CameraPropertyN>
              * </CameraInstance>
              *</CameraRoot>
              * CameraPropertyX Tags are the constant property keys defined in CameraProperties.h
              */

            name = xmlTextReaderConstName(reader);
            ret = xmlTextReaderRead(reader);
            if ( name && ( strcmp( (const char *) name, CAMERA_ROOT) == 0 ) && ( 1 == ret)) {

                // Start parsing the Camera Instances
                ret = xmlTextReaderRead(reader);
                name = xmlTextReaderConstName(reader);

                // skip #text tag
                xmlTextReaderRead(reader);
                if ( 1 != ret) {
                    CAMHAL_LOGEA("XML File Reached end prematurely or parsing error");
                    return ret;
                }

                // Reset the # of CamerasSupported that was defined in the initial setup
                mCamerasSupported = 0;

                while( name && strcmp( (const char *) name, CAMERA_INSTANCE) == 0) {
                    CAMHAL_LOGDB("Camera Element Name:%s", name);

                    curCameraIndex  = mCamerasSupported;

                    // Increment the number of cameras supported
                    mCamerasSupported++;
                    CAMHAL_LOGDB("Incrementing the number of cameras supported %d",mCamerasSupported);

                    while (1) {
                        propertyIndex = 0;

                        if ( NULL == nextName ) {
                            ret = xmlTextReaderRead(reader);
                            if ( 1 != ret) {
                                CAMHAL_LOGEA("XML File Reached end prematurely or parsing error");
                                break;
                            }
                            // Get the tag name
                            name = xmlTextReaderConstName(reader);
                            CAMHAL_LOGDB("Found tag:%s", name);
                        }
                        else {
                            name = nextName;
                        }

                        // Check if the tag is CameraInstance, if so we are done with properties for current camera
                        // Move on to next one if present
                        if ( name && strcmp((const char *) name, "CameraInstance") == 0) {
                            // End of camera element
                            if ( xmlTextReaderNodeType(reader) == XML_READER_TYPE_END_ELEMENT ) {
                                CAMHAL_LOGDA("CameraInstance close tag found");
                            }
                            else {
                                CAMHAL_LOGDA("CameraInstance close tag not found. Please check properties XML file");        
                            }
                            break;

                        }

                        // The next tag should be #text and should contain the value, else process it next time around as regular tag
                        ret = xmlTextReaderRead(reader);
                        if(1 != ret) {
                            CAMHAL_LOGDA("XML File Reached end prematurely or parsing error");
                            break;
                        }

                        // Get the next tag name
                        nextName = xmlTextReaderConstName(reader);
                        CAMHAL_LOGDB("Found next tag:%s", name);
                        if ( nextName && strcmp((const char *) nextName, TEXT_XML_ELEMENT) == 0) {
                            nextName = NULL;
                            value = xmlTextReaderConstValue(reader);
                            if (value)
                              strncpy(val, (const char *) value, sizeof(val)-1);
                            value = (const xmlChar *) val;
                            CAMHAL_LOGDB("Found next tag value: %s", value);
                        }

                        // Read the closing tag
                        ret = xmlTextReaderRead(reader);
                        if ( xmlTextReaderNodeType(reader) == XML_READER_TYPE_END_ELEMENT ) {
                            CAMHAL_LOGDA("Found matching ending tag");
                        }
                        else {
                            CAMHAL_LOGDA("Couldn't find matching ending tag");
                        }
                        ///Read the #text tag for closing tag
                        ret = xmlTextReaderRead(reader);

                        CAMHAL_LOGDB("Tag Name %s Tag Value %s", name, value);
                        if (name) {
                            // ret = mCameraProps[curCameraIndex].set((const char *) name, (const char *) value);
                            mCameraProps[curCameraIndex].set((const char *) name, (const char *) value);
                            CAMHAL_LOGDB("[%d] Set value for key %s to value %s", curCameraIndex, name, value );
#if 0
                            if(NO_ERROR != ret) {
                                CAMHAL_LOGEB("[%d] Cannot set value for key %s value %s (%d)", curCameraIndex, name, value, ret );
                                // LOG_FUNCTION_NAME_EXIT
                                // return ret;
                            } else {
                                CAMHAL_LOGDB("[%d] Set value for key %s to value %s", curCameraIndex, name, value );
                            }
#endif
                        }
                    }

                    //skip #text tag
                    ret = xmlTextReaderRead(reader);
                    if (1 != ret) {
                        CAMHAL_LOGDA("Completed parsing the XML file");
                        ret = NO_ERROR;
                        goto exit;
                    }

                    ret = xmlTextReaderRead(reader);
                    if (1 != ret) {
                        CAMHAL_LOGDA("Completed parsing the XML file");
                        ret = NO_ERROR;
                        goto exit;
                    }

                    ///Get the tag name
                    name = xmlTextReaderConstName(reader);
                    nextName = NULL;
                    CAMHAL_LOGDB("Found tag:%s", name);
                    if ( name && ( xmlTextReaderNodeType(reader) == XML_READER_TYPE_END_ELEMENT ) && ( strcmp( (const char *) name, CAMERA_ROOT) == 0 ) ) {
                        CAMHAL_LOGDA("Completed parsing the XML file");
                        ret = NO_ERROR;
                        goto exit;
                    }

                    //skip #text tag
                    xmlTextReaderRead(reader);
                    if(1 != ret) {
                        CAMHAL_LOGDA("Completed parsing the XML file");
                        ret = NO_ERROR;
                        goto exit;
                    }
                }
            }

        }
    }
    else {
        CAMHAL_LOGEB("Unable to open %s\n", file);
    }
exit:

    CAMHAL_LOGDA("Freeing the XML Reader");
    xmlFreeTextReader(reader);

    LOG_FUNCTION_NAME_EXIT
    return ret;
}


status_t CameraProperties::storeXMLProperties() {
    LOG_FUNCTION_NAME

    xmlTextWriterPtr writer;
    status_t ret = NO_ERROR;
    // CameraProperty **prop = NULL;

#if 0
    writer = xmlNewTextWriterFilename( ( const char * ) mXMLFullPath, 0);
    if ( NULL != writer ) {

        if ( NO_ERROR == ret ) {

            ret = xmlTextWriterStartDocument(writer, NULL, "ISO-8859-1", NULL);
            if ( 0 > ret ) {
                CAMHAL_LOGEB("Error starting XML document 0x%x", ret);
                }
            }

        if ( NO_ERROR == ret ) {
            //Insert CameraRoot Tag
            ret = xmlTextWriterStartElement(writer, ( const xmlChar * ) CAMERA_ROOT);
            ret |= xmlTextWriterWriteFormatString(writer, "\n");
            if ( 0 > ret )
                {
                CAMHAL_LOGEB("Error inserting root tag 0x%x", ret);
                }
            }

        if ( NO_ERROR == ret ) {

            for ( int i = 0 ; i < mCamerasSupported ; i++ )
                {

                prop = getProperties(i);
                if ( NULL == prop )
                    {
                    break;
                    }

                //Insert CameraIntance Tag
                ret = xmlTextWriterStartElement(writer, ( const xmlChar * ) CAMERA_INSTANCE);
                ret |= xmlTextWriterWriteFormatString(writer, "\n");
                if ( 0 > ret )
                    {
                    CAMHAL_LOGEB("Error inserting camera instance tag 0x%x", ret);
                    break;
                    }

                    for ( int j = 1 ; j < PROP_INDEX_MAX ; j++ )
                        {

                    if ( ( NULL != prop[j]->mPropName ) &&
                          ( NULL != prop[j]->mPropValue) )
                        {

                        //Insert individual Tags
                        ret = xmlTextWriterWriteElement(writer, ( const xmlChar * ) prop[j]->mPropName,
                                                                             ( const xmlChar * ) prop[j]->mPropValue);
                        ret |= xmlTextWriterWriteFormatString(writer, "\n");
                        if ( 0 > ret )
                            {
                            CAMHAL_LOGEB("Error inserting tag %s 0x%x", ( const char * ) prop[j]->mPropName,
                                                                                                  ret);
                            break;
                            }
                        }
                    }

                if ( 0 > ret )
                    {
                    break;
                    }

                //CameraInstance ends here
                ret = xmlTextWriterEndElement(writer);
                ret |= xmlTextWriterWriteFormatString(writer, "\n");
                if ( 0 > ret )
                    {
                    CAMHAL_LOGEB("Error inserting camera instance tag 0x%x", ret);
                    }

                }

            }

        if ( NO_ERROR == ret )
            {
            //CameraRoot ends here
            ret = xmlTextWriterEndElement(writer);
            ret |= xmlTextWriterWriteFormatString(writer, "\n");
            if ( 0 > ret )
                {
                CAMHAL_LOGEB("Error inserting root tag 0x%x", ret);
                }
            }


        if ( NO_ERROR == ret ) {
            ret = xmlTextWriterEndDocument(writer);
            if ( 0 > ret )
                {
                CAMHAL_LOGEB("Error closing xml document 0x%x", ret);
                }
        }

    }
    else {
        CAMHAL_LOGEB("Unable to open %s\n", mXMLFullPath);
    }

    CAMHAL_LOGVA("Freeing the XML Reader");
    xmlFreeTextWriter(writer);
#endif

    LOG_FUNCTION_NAME_EXIT
    return ret;
}

};
