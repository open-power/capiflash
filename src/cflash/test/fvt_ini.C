// IBM_PROLOG_BEGIN_TAG IBM_PROLOG_END
/**
 *******************************************************************************
 * \file
 * \brief
 * \ingroup
 ******************************************************************************/
#include <gtest/gtest.h>

#include <string>
extern "C"
{
    #include <prov.h>
    
    int32_t                 g_traceE = 1;   /* error traces */
    int32_t                 g_traceI = 1;       /* informative 'where we are in code' traces */
    int32_t                 g_traceF = 1;       /* function exit/enter */
    int32_t                 g_traceV = 1;       /* verbose trace...lots of information */
}

#define HOST_ID 1
#define STR_HOST_ID "1"


TEST(Provisioning_FVT_Suite, IniSectionIsValid)
{
    EXPECT_FALSE(section_is_valid((char*)"invalid;semicolon"));
    EXPECT_FALSE(section_is_valid((char*)"invalid inner\nspaces"));
    EXPECT_FALSE(section_is_valid((char*)"invalidinner\nnewline"));
    EXPECT_FALSE(section_is_valid((char*)"invalidtrailingspaces\r"));
    EXPECT_FALSE(section_is_valid((char*)"\tinvalidleadingspaces"));
    EXPECT_FALSE(section_is_valid((char*)"\t"));
    EXPECT_FALSE(section_is_valid((char*)" "));
    EXPECT_FALSE(section_is_valid((char*)""));
    EXPECT_FALSE(section_is_valid((char*)";"));
    EXPECT_FALSE(section_is_valid((char*)NULL));
}

TEST(Provisioning_FVT_Suite, IniTrimWhitespace)
{
    char buffer[80] = "middle space";
    EXPECT_STREQ("middle", trim_whitespace_inplace(buffer));
    EXPECT_STREQ("middle", buffer);
    strcpy(buffer,"mid     space");
    EXPECT_STREQ("mid", trim_whitespace_inplace(buffer));
    strcpy(buffer,"tab\tspace");
    EXPECT_STREQ("tab", trim_whitespace_inplace(buffer));
    strcpy(buffer," spacetab\tspace");
    EXPECT_STREQ("spacetab", trim_whitespace_inplace(buffer));
    strcpy(buffer," \t   padded stuff");
    EXPECT_STREQ("padded", trim_whitespace_inplace(buffer));
    strcpy(buffer," \t   ");
    EXPECT_STREQ("", trim_whitespace_inplace(buffer));
    strcpy(buffer," ");
    EXPECT_STREQ("", trim_whitespace_inplace(buffer));
    strcpy(buffer,"");
    EXPECT_STREQ("", trim_whitespace_inplace(buffer));
    EXPECT_EQ(NULL, trim_whitespace_inplace(NULL));
}

TEST(Provisioning_FVT_Suite, IniTrimComments)
{
    char buffer[80] = "middle space";
    trim_comments(buffer);
    EXPECT_STREQ("middle space", buffer);
    strcpy(buffer,"mid     space;");
    trim_comments(buffer);
    EXPECT_STREQ("mid     space", buffer);
    strcpy(buffer,"mid  ;   space");
    trim_comments(buffer);
    EXPECT_STREQ( "mid  ", buffer);
    strcpy(buffer,"mid;   space");
    trim_comments(buffer);
    EXPECT_STREQ( "mid", buffer);
    strcpy(buffer,"mi;   space");
    trim_comments(buffer);
    EXPECT_STREQ( "mi", buffer);
    strcpy(buffer,"m;   space");
    trim_comments(buffer);
    EXPECT_STREQ( "m", buffer);
    strcpy(buffer,";test   space");
    trim_comments(buffer);
    EXPECT_STREQ( "", buffer);
}


TEST(Provisioning_FVT_Suite, IniCreateRootEntry)
{
    ini_dict_t* ini_entry = NULL;
    ini_dict_t* cur_entry = NULL;
    append_ini_entry(&ini_entry, (char*)"section", (char*)"key", (char*)"value");
    ASSERT_TRUE((ini_entry != NULL));
    EXPECT_STREQ("section",ini_entry->section);
    EXPECT_STREQ("key",ini_entry->key);
    EXPECT_STREQ("value",ini_entry->value);
    ASSERT_TRUE((ini_entry->next == NULL));
    cur_entry = append_ini_entry(&ini_entry, (char*)"sec_section", (char*)"sec_key", (char*)"sec_value");
    ASSERT_TRUE((cur_entry != NULL));
    EXPECT_STREQ("sec_section",cur_entry->section);
    EXPECT_STREQ("sec_key",cur_entry->key);
    EXPECT_STREQ("sec_value",cur_entry->value);
    ASSERT_TRUE((cur_entry->next == NULL));
    cur_entry = append_ini_entry(&ini_entry, (char*)"3rd_sec", (char*)"3rd_key", (char*)"3rd_val");
    ASSERT_TRUE((cur_entry != NULL));
    EXPECT_STREQ("3rd_sec",cur_entry->section);
    EXPECT_STREQ("3rd_key",cur_entry->key);
    EXPECT_STREQ("3rd_val",cur_entry->value);
    ASSERT_TRUE((cur_entry->next == NULL));
}


TEST(Provisioning_FVT_Suite, COMPARE_WWPNS_ARRAY1_NULL)
{
    prov_wwpn_info_t * wwpn_array_1 = NULL;
    uint16_t num_wwpns_1 = 1;
    prov_wwpn_info_t wwpn_array_2[MAX_WWPNS];
    uint16_t num_wwpns_2;
    
    provGetAllWWPNs(wwpn_array_2, &num_wwpns_2);
    
    int rv = compareWWPNs(wwpn_array_1, wwpn_array_2, num_wwpns_1, num_wwpns_2);

    ASSERT_EQ(rv, -1);
}

TEST(Provisioning_FVT_Suite, COMPARE_WWPNS_ARRAY2_NULL)
{
    prov_wwpn_info_t wwpn_array_1[MAX_WWPNS];
    uint16_t num_wwpns_1;
    prov_wwpn_info_t * wwpn_array_2 = NULL;
    uint16_t num_wwpns_2 = 1;
    
    provGetAllWWPNs(wwpn_array_1, &num_wwpns_1);
    provGetAllWWPNs(wwpn_array_2, &num_wwpns_2);
    
    int rv = compareWWPNs(wwpn_array_1, wwpn_array_2, num_wwpns_1, num_wwpns_2);

    ASSERT_EQ(rv, -1);
}

TEST(Provisioning_FVT_Suite, COMPARE_WWPNS_0_NUM_1)
{
    prov_wwpn_info_t wwpn_array_1[MAX_WWPNS];
    uint16_t num_wwpns_1;
    prov_wwpn_info_t wwpn_array_2[MAX_WWPNS];
    uint16_t num_wwpns_2;
    
    provGetAllWWPNs(wwpn_array_1, &num_wwpns_1);
    num_wwpns_1 = 0;
    provGetAllWWPNs(wwpn_array_2, &num_wwpns_2);
    
    int rv = compareWWPNs(wwpn_array_1, wwpn_array_2, num_wwpns_1, num_wwpns_2);

    ASSERT_EQ(rv, -1);
}

TEST(Provisioning_FVT_Suite, COMPARE_WWPNS_0_NUM_2)
{
    prov_wwpn_info_t wwpn_array_1[MAX_WWPNS];
    uint16_t num_wwpns_1;
    prov_wwpn_info_t wwpn_array_2[MAX_WWPNS];
    uint16_t num_wwpns_2;
    
    provGetAllWWPNs(wwpn_array_1, &num_wwpns_1);
    provGetAllWWPNs(wwpn_array_2, &num_wwpns_2);
    num_wwpns_2 = 0;
    
    int rv = compareWWPNs(wwpn_array_1, wwpn_array_2, num_wwpns_1, num_wwpns_2);

    ASSERT_EQ(rv, -1);
}

TEST(Provisioning_FVT_Suite, CONCATENATE_WWPNS_VALID_INPUT)
{
    prov_wwpn_info_t wwpn_array[MAX_WWPNS];
    uint16_t num_wwpns;
    
    strcpy(wwpn_array[0].wwpn, "0000000000000000");
    strcpy(wwpn_array[1].wwpn, "AAAAAAAAAAAAAAAA");
    strcpy(wwpn_array[2].wwpn, "BBBBBBBBBBBBBBBB");
    num_wwpns = 3;

    char concat_wwpns[WWPN_STRING_SIZE];
    bzero(concat_wwpns, WWPN_STRING_SIZE);

    int rv = concatenateWWPNs(wwpn_array, num_wwpns, concat_wwpns);

    ASSERT_EQ(rv, 0);
    ASSERT_STREQ("0000000000000000:AAAAAAAAAAAAAAAA:BBBBBBBBBBBBBBBB", concat_wwpns);
}

TEST(Provisioning_FVT_Suite, CONCATENATE_WWPNS_NULL_ARRAY)
{
    prov_wwpn_info_t * wwpn_array = NULL;
    uint16_t num_wwpns = 3;
    
    char concat_wwpns[WWPN_STRING_SIZE];
    bzero(concat_wwpns, WWPN_STRING_SIZE);

    int rv = concatenateWWPNs(wwpn_array, num_wwpns, concat_wwpns);

    ASSERT_EQ(rv, -1);
}

TEST(Provisioning_FVT_Suite, CONCATENATE_WWPNS_0_NUM)
{
    prov_wwpn_info_t wwpn_array[MAX_WWPNS];
    uint16_t num_wwpns;
    
    strcpy(wwpn_array[0].wwpn, "0000000000000000");
    strcpy(wwpn_array[1].wwpn, "AAAAAAAAAAAAAAAA");
    strcpy(wwpn_array[2].wwpn, "BBBBBBBBBBBBBBBB");
    num_wwpns = 0;

    char concat_wwpns[WWPN_STRING_SIZE];
    bzero(concat_wwpns, WWPN_STRING_SIZE);

    int rv = concatenateWWPNs(wwpn_array, num_wwpns, concat_wwpns);

    ASSERT_EQ(rv, 0);
}

TEST(Provisioning_FVT_Suite, CONCATENATE_WWPNS_VALID_NULL_CONCAT_WWPNS)
{
    prov_wwpn_info_t wwpn_array[MAX_WWPNS];
    uint16_t num_wwpns;
    
    strcpy(wwpn_array[0].wwpn, "0000000000000000");
    strcpy(wwpn_array[1].wwpn, "AAAAAAAAAAAAAAAA");
    strcpy(wwpn_array[2].wwpn, "BBBBBBBBBBBBBBBB");
    num_wwpns = 3;

    char * concat_wwpns = NULL;

    int rv = concatenateWWPNs(wwpn_array, num_wwpns, concat_wwpns);

    ASSERT_EQ(rv, -1);
}
