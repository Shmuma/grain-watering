#ifndef __HISTORY_H__
#define __HISTORY_H__


enum history_stage_t {
    HS_Stage1 = 0,
    HS_Stage2,
    HS_Stage3,
    HS_Stage4,
    HS_Events,
    HS_Cleanings,
};


enum history_kind_t {
    HK_Humidity,
    HK_GrainFlow,
    HK_GrainTemp,
    HK_GrainNature,
    HK_WaterPress,
    HK_TargetHumidity,
    HK_WaterFlow,
    HK_Setting,
};


#endif
