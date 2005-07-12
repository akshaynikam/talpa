/*
 * operation_excl.c
 *
 * TALPA Filesystem Interceptor
 *
 * Copyright (C) 2004 Sophos Plc, Oxford, England.
 *
 * This program is free software; you can redistribute it and/or modify it under the terms of the
 * GNU General Public License Version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
 * even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with this program; if not,
 * write to the Free Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 */
#include <linux/kernel.h>

#include <linux/slab.h>
#include <linux/string.h>
#include <asm/fcntl.h>

#define TALPA_SUBSYS "opexcl"
#include "common/talpa.h"
#include "operation_excl.h"

/*
 * Forward declare implementation methods.
 */
static void examineFile(const void* self, IEvaluationReport* report, const IPersonality* userInfo, const IFileInfo* info, IFile* file);
static void examineFilesystem(const void* self, IEvaluationReport* report, const IPersonality* userInfo, const IFilesystemInfo* info);
static bool enable(void* self);
static void disable(void* self);
static bool isEnabled(const void* self);
static const char* configName(const void* self);
static const PODConfigurationElement* allConfig(const void* self);
static const char* config(const void* self, const char* name);
static void setConfig(void* self, const char* name, const char* value);
static void deleteOperationExclusionProcessor(struct tag_OperationExclusionProcessor* object);

/*
 * Constants
 */
#define CFG_STATUS          "status"
#define CFG_ONLYBLOCKDEV    "fs-onlyblock"
#define CFG_VALUE_ENABLED   "enabled"
#define CFG_VALUE_DISABLED  "disabled"
#define CFG_ACTION_ENABLE   "enable"
#define CFG_ACTION_DISABLE  "disable"

/*
 * Template Object.
 */
static OperationExclusionProcessor template_OperationExclusionProcessor =
    {
        {
            examineFile,
            examineFilesystem,
            enable,
            disable,
            isEnabled,
            0,
            (void (*)(void*))deleteOperationExclusionProcessor
        },
        {
            configName,
            allConfig,
            config,
            setConfig,
            0,
            (void (*)(void*))deleteOperationExclusionProcessor
        },
        deleteOperationExclusionProcessor,
        true,
        true,
        TALPA_MUTEX_INIT,
        {
            {0, 0, OPERATIONEXCLUSIONPROCESSOR_CFGDATASIZE, true, true },
            {0, 0, OPERATIONEXCLUSIONPROCESSOR_CFGDATASIZE, true, true },
            {0, 0, 0, false, false }
        },
        {
            { CFG_STATUS, CFG_VALUE_ENABLED }
        },
        {
            { CFG_ONLYBLOCKDEV, CFG_VALUE_ENABLED }
        }

    };
#define this    ((OperationExclusionProcessor*)self)



/*
 * Object creation/destruction.
 */
OperationExclusionProcessor* newOperationExclusionProcessor(void)
{
    OperationExclusionProcessor* object;


    object = kmalloc(sizeof(template_OperationExclusionProcessor), SLAB_KERNEL);
    if (object != 0)
    {
        memcpy(object, &template_OperationExclusionProcessor, sizeof(template_OperationExclusionProcessor));
        object->i_IInterceptFilter.object = object->i_IConfigurable.object = object;

        talpa_mutex_init(&object->mConfigSerialize);

        object->mConfig[0].name  = object->mConfigStatus[0].name;
        object->mConfig[0].value = object->mConfigStatus[0].value;
        object->mConfig[1].name  = object->mConfigOnlyBlockDev[0].name;
        object->mConfig[1].value = object->mConfigOnlyBlockDev[0].value;
    }
    return object;
}

static void deleteOperationExclusionProcessor(struct tag_OperationExclusionProcessor* object)
{
    kfree(object);
    return;
}

/*
 * IInterceptFilter.
 */
static void examineFile(const void* self, IEvaluationReport* report, const IPersonality* userInfo, const IFileInfo* info, IFile* file)
{
    switch (info->operation(info))
    {
        case EFS_Open:
            /*
             * Allow if file was truncated on open, is write only
             * or is open for exclusive creation.
             */
            if ((info->flags(info) & (O_WRONLY | O_TRUNC))
                || ((info->flags(info) & (O_CREAT | O_EXCL)) == (O_CREAT | O_EXCL))
               )
            {
                report->setRecommendedAction(report, EIA_Allow);
            }
            break;
        case EFS_Close:
            /*
             * Allow if we are closing and read-only file.
             */
            if ( ! info->isWritable(info) )
            {
                report->setRecommendedAction(report, EIA_Allow);
            }
            break;
        default:
            /*
             * Ignore the operation.
             */
            break;
    }
    return;
}

static void examineFilesystem(const void* self, IEvaluationReport* report, const IPersonality* userInfo, const IFilesystemInfo* info)
{
    if ( this->mOnlyBlockDev && !info->device(info) )
    {
        report->setRecommendedAction(report, EIA_Allow);
    }

    return;
}

static bool enable(void* self)
{
    if (!this->mEnabled)
    {
        this->mEnabled = true;
        strcpy(this->mConfig[0].value, CFG_VALUE_ENABLED);
        info("Enabled");
    }
    return true;
}

static void disable(void* self)
{
    if (this->mEnabled)
    {
        this->mEnabled = false;
        strcpy(this->mConfig[0].value, CFG_VALUE_DISABLED);
        info("Disabled");
    }
    return;
}

static void onlyBlockDev(void* self, bool state)
{
    if ( !this->mOnlyBlockDev && state )
    {
        this->mOnlyBlockDev = true;
        strcpy(this->mConfig[1].value, CFG_VALUE_ENABLED);
        info("Ignoring non-block device mount operations");
    }
    else if ( this->mOnlyBlockDev && !state )
    {
        this->mOnlyBlockDev = false;
        strcpy(this->mConfig[1].value, CFG_VALUE_DISABLED);
        info("Intercepting all mount operations");
    }
    return;
}

static bool isEnabled(const void* self)
{
    return this->mEnabled;
}

/*
 * IConfigurable.
 */
static const char* configName(const void* self)
{
    return "OperationExclusionProcessor";
}

static const PODConfigurationElement* allConfig(const void* self)
{
    return this->mConfig;
}

static const char* config(const void* self, const char* name)
{
    PODConfigurationElement*    cfgElement;


    /*
     * Find the named item.
     */
    for (cfgElement = this->mConfig; cfgElement->name != 0; cfgElement++)
    {
        if (strcmp(name, cfgElement->name) == 0)
        {
            break;
        }
    }

    /*
     * Return what was found else a null pointer.
     */
    if (cfgElement->name != 0)
    {
        return cfgElement->value;
    }
    return 0;
}

static void  setConfig(void* self, const char* name, const char* value)
{
   PODConfigurationElement*    cfgElement;


    /*
     * Find the named item.
     */
    for (cfgElement = this->mConfig; cfgElement->name != 0; cfgElement++)
    {
        if (strcmp(name, cfgElement->name) == 0)
        {
            break;
        }
    }

    /*
     * Cant set that which does not exist!
     */
    if (cfgElement->name == 0)
    {
        return;
    }

    /*
     * OK time to do some work...
     */
    if (strcmp(name, CFG_STATUS) == 0)
    {
        if (strcmp(value, CFG_ACTION_ENABLE) == 0)
        {
            talpa_mutex_lock(&this->mConfigSerialize);
            enable(this);
            talpa_mutex_unlock(&this->mConfigSerialize);
        }
        else if (strcmp(value, CFG_ACTION_DISABLE) == 0)
        {
            talpa_mutex_lock(&this->mConfigSerialize);
            disable(this);
            talpa_mutex_unlock(&this->mConfigSerialize);
        }
    }
    else if (strcmp(name, CFG_ONLYBLOCKDEV) == 0)
    {
        if (strcmp(value, CFG_ACTION_ENABLE) == 0)
        {
            talpa_mutex_lock(&this->mConfigSerialize);
            onlyBlockDev(this, true);
            talpa_mutex_unlock(&this->mConfigSerialize);
        }
        else if (strcmp(value, CFG_ACTION_DISABLE) == 0)
        {
            talpa_mutex_lock(&this->mConfigSerialize);
            onlyBlockDev(this, false);
            talpa_mutex_unlock(&this->mConfigSerialize);
        }
    }
    return;
}

/*
 * End of operation_excl.c
 */

