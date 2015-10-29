/*                                                                        */
/* Contributors Listed Below - COPYRIGHT 2014,2015                        */
/* [+] International Business Machines Corp.                              */
/*                                                                        */
/*                                                                        */
/* Licensed under the Apache License, Version 2.0 (the "License");        */
/* you may not use this file except in compliance with the License.       */
/* You may obtain a copy of the License at                                */
/*                                                                        */
/*     http://www.apache.org/licenses/LICENSE-2.0                         */
/*                                                                        */
/* Unless required by applicable law or agreed to in writing, software    */
/* distributed under the License is distributed on an "AS IS" BASIS,      */
/* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or        */
/* implied. See the License for the specific language governing           */
/* permissions and limitations under the License.                         */
/*                                                                        */
/* IBM_PROLOG_END_TAG                                                     */
#include <kv_trace.h>

KV_Trace_t  fvt_kv_trace;
KV_Trace_t *pFT = &fvt_kv_trace;

void fvt_trace_intHandler(int s)
{
    if (NULL == pFT)       _exit(-2);
    if (EYEC_INVALID(pFT)) _exit(-3);

    if (pFT->logfp)
    {
        fprintf(pFT->logfp,"---------------------------------------------------------------------------\n");
        fprintf(pFT->logfp,"DONE, signal handler: Date is %s at %s\n",__DATE__,__TIME__);
        fprintf(pFT->logfp,"---------------------------------------------------------------------------\n");
        fflush (pFT->logfp);
        fclose (pFT->logfp);
    }
    _exit(-1);
}
