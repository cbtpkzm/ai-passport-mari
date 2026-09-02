#include "pet_app.h"

#include "bsp_battery.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_random.h"
#include "esp_system.h"
#include "lvgl.h"
#include "miniz.h"
#include "nvs.h"
#include "pet_state.h"
#include "ui_pixel.h"
#include <stddef.h>
#include <string.h>

LV_FONT_DECLARE(lv_font_pet_zh_14);

static const char *TAG = "pet_app";

extern const uint8_t pet_idle_start[] asm("_binary_idle_zframes_start");
extern const uint8_t pet_idle_end[] asm("_binary_idle_zframes_end");
extern const uint8_t pet_happy_start[] asm("_binary_happy_zframes_start");
extern const uint8_t pet_happy_end[] asm("_binary_happy_zframes_end");
extern const uint8_t pet_sad_start[] asm("_binary_sad_zframes_start");
extern const uint8_t pet_sad_end[] asm("_binary_sad_zframes_end");
extern const uint8_t pet_surprised_start[] asm("_binary_surprised_zframes_start");
extern const uint8_t pet_surprised_end[] asm("_binary_surprised_zframes_end");
extern const uint8_t pet_play_start[] asm("_binary_play_zframes_start");
extern const uint8_t pet_play_end[] asm("_binary_play_zframes_end");
extern const uint8_t pet_rest_start[] asm("_binary_rest_zframes_start");
extern const uint8_t pet_rest_end[] asm("_binary_rest_zframes_end");
extern const uint8_t pet_cool_start[] asm("_binary_cool_zframes_start");
extern const uint8_t pet_cool_end[] asm("_binary_cool_zframes_end");

typedef enum {
    PET_ASSET_IDLE = 0,
    PET_ASSET_HAPPY,
    PET_ASSET_SAD,
    PET_ASSET_SURPRISED,
    PET_ASSET_PLAY,
    PET_ASSET_REST,
    PET_ASSET_COOL,
    PET_ASSET_COUNT,
} pet_asset_t;

typedef struct {
    const uint8_t *start;
    const uint8_t *end;
} embedded_asset_t;

static const embedded_asset_t ASSET_DATA[PET_ASSET_COUNT] = {
    [PET_ASSET_IDLE] = { pet_idle_start, pet_idle_end },
    [PET_ASSET_HAPPY] = { pet_happy_start, pet_happy_end },
    [PET_ASSET_SAD] = { pet_sad_start, pet_sad_end },
    [PET_ASSET_SURPRISED] = { pet_surprised_start, pet_surprised_end },
    [PET_ASSET_PLAY] = { pet_play_start, pet_play_end },
    [PET_ASSET_REST] = { pet_rest_start, pet_rest_end },
    [PET_ASSET_COOL] = { pet_cool_start, pet_cool_end },
};

#define PET_FRAME_WIDTH 96
#define PET_FRAME_HEIGHT 128
#define PET_FRAME_BYTES (PET_FRAME_WIDTH * PET_FRAME_HEIGHT * 2)
#define PET_ASSET_HEADER_BYTES 12

static const char *ACTION_NAMES[PET_ACTION_COUNT] = { "吃饭", "玩耍", "休息" };

typedef struct {
    const char *const *lines;
    size_t count;
} dialogue_bank_t;

static const char *const IDLE_DAWN_LINES[] = {
    "天边开始变亮啦。\n今天会发生什么呢？",
    "早上的风凉凉的。\n一下就把我吹醒了。",
    "你看，云边是金色的！\n太阳马上就要出来啦。",
    "街上还没有什么人。\n我们像是第一个醒来的。",
    "昨晚的梦还没忘掉。\n要不要讲给你听？",
    "小鸟已经开始说话了。\n可惜我听不懂它们。",
    "早呀！你醒得比我晚。\n这次是我赢了哦。",
    "空气闻起来湿湿的。\n今天也许会下雨吧。",
    "新的一天刚刚打开。\n我们先做什么好呢？",
    "太阳偷偷露出一点啦。\n我早就发现它了。",
};

static const char *const IDLE_DAY_LINES[] = {
    "太阳暖暖的。\n站一会儿就不想走啦。",
    "那朵云像不像兔子？\n它还在慢慢变大。",
    "今天的风跑得好快。\n我的头发都乱了。",
    "外面这么亮。\n一定藏着好多好玩的事。",
    "我看到一只小鸟飞过去。\n它是不是也在赶路？",
    "影子缩得小小的。\n太阳快到头顶啦。",
    "要不要陪我说会儿话？\n我有好多事情想讲。",
    "这边看看，那边看看。\n今天怎么都看不够。",
    "路上好像很热闹。\n我们也去凑个热闹吧。",
    "天气这么好。\n一直待着多浪费呀。",
};

static const char *const IDLE_DUSK_LINES[] = {
    "天空变成橘色啦。\n像打翻了一杯果汁。",
    "太阳要回家了吗？\n它明天还会来的。",
    "影子被拉得好长。\n看起来比我还高。",
    "晚风比白天温柔多了。\n吹得人懒洋洋的。",
    "云朵边上亮晶晶的。\n好像偷偷点了灯。",
    "今天快要结束了。\n你最喜欢哪一件事？",
    "鸟都往同一个方向飞。\n它们也要回家吧。",
    "城里的灯一盏盏亮了。\n像有人在眨眼睛。",
    "天还没有完全黑。\n我们再玩一小会儿吧。",
    "黄昏一下就会过去。\n所以我要认真看着。",
};

static const char *const IDLE_NIGHT_LINES[] = {
    "我刚刚数了三遍星星。\n每次数出来都不一样。",
    "你看，远处还亮着灯。\n那边的人也没睡吧。",
    "今晚的月亮圆圆的。\n像一块咬不到的饼。",
    "夜里的风有点凉。\n你再靠近一点嘛。",
    "城里终于安静下来啦。\n现在能听见虫叫了。",
    "那颗星刚才闪了一下。\n它是不是在跟我说话？",
    "天这么黑，路还在这里。\n慢慢走就不会丢啦。",
    "我才没有偷偷发呆。\n我是在认真看夜景。",
    "大家都睡着以后。\n会不会做同一个梦呀？",
    "再陪我待一小会儿吧。\n我还不想跟今天说再见。",
};

static const char *const HAPPY_LINES[] = {
    "你来啦！\n我刚好有好多开心要分给你。",
    "嘿嘿，我今天超有精神！\n感觉能一口气跑到屋顶。",
    "不知道为什么特别开心。\n可能是因为你在这里吧。",
    "今天做什么都会成功的！\n不信我们现在就试试。",
    "快点快点，我等不及啦。\n前面一定有好玩的事。",
    "我要把好心情分你一半。\n这样我们就一样开心了。",
    "再夸我一句也不是不行。\n不过只能说实话哦。",
    "要不要比赛谁先笑出来？\n我可不会故意让你。",
    "你走前面吧。\n我会从后面偷偷超过你。",
    "今天的风也在帮我。\n跑起来轻飘飘的。",
    "我刚刚想到一个好主意！\n先不告诉你是什么。",
    "跟你一起的话。\n去哪儿好像都很好玩。",
};

static const char *const SAD_LINES[] = {
    "唔，我今天有点没力气。\n给我一点好吃的就行。",
    "刚刚还好好的。\n怎么突然就有点累了。",
    "我才没有不开心呢……\n只是想让你陪我坐一会儿。",
    "两条腿都变得沉沉的。\n今天慢一点也没关系吧？",
    "风吹过来凉飕飕的。\n你别离我太远哦。",
    "我休息一下马上就好。\n真的，不会让你等很久。",
    "等我充好电。\n我们再一起出去玩。",
    "今天好像什么都做不好。\n不过明天可以再试一次。",
    "我不想说话的时候。\n你安静陪着我就好。",
    "鼻子有一点酸酸的。\n但我才不会随便哭呢。",
    "让我靠一会儿嘛。\n就一小会儿，好不好？",
    "今天先到这里吧。\n明天醒来又会有精神啦。",
};

static const char *const COOL_LINES[] = {
    "看到没有？刚才超帅的！\n你是不是看呆啦？",
    "哼哼，这招我练了好多次。\n当然不会失手。",
    "我就说我很厉害吧！\n这次你总该相信了。",
    "其实也没有很难啦。\n只是我学得比较快。",
    "这个动作要保密哦。\n别人问起就说不知道。",
    "别眨眼。\n我的下一招还会更快。",
    "夸我也可以。\n不过要小声一点哦。",
    "我才没有得意忘形呢。\n我只是稍微开心一下。",
    "连衣角都没有碰到！\n这次可漂亮了。",
    "下一次换个更难的吧。\n太简单就没意思啦。",
    "这是我们两个人的绝招。\n不可以随便教给别人。",
    "刚才那一下不算运气。\n再来十次我也做得到。",
};

static const char *const FEED_LINES[] = {
    "哇，是给我的吗？\n那我就不客气啦！",
    "好香！我一下就闻到了。\n肚子也跟着叫起来了。",
    "你怎么知道我刚好饿啦？\n难道你听见我肚子叫了？",
    "你居然记得我爱吃这个。\n我、我有一点开心哦。",
    "最后一口留给你好不好？\n不过你只能吃一小口。",
    "我会慢慢吃的。\n才不会因为着急噎到呢。",
    "吃饱以后要陪我玩哦。\n这是我们说好的。",
    "再来一点点嘛。\n真的就只要一点点。",
    "嘿嘿，肚子暖起来了。\n整个人也有精神啦。",
    "这一份看起来特别好吃。\n我应该先从哪里下口呢？",
    "谢谢你！这次是真心的。\n下次也要记得我哦。",
    "我现在又有好多力气啦！\n马上就能跑给你看。",
};

static const char *const PLAY_LINES[] = {
    "来比赛吧，我肯定不会输！\n你可不要故意让我哦。",
    "快快快，我们出去玩！\n我已经想好要去哪里了。",
    "再来一局嘛，就一局！\n这次我一定能赢。",
    "输的人要讲一个笑话！\n不许讲听过的哦。",
    "我先跑。\n你数到三再来追我。",
    "这次我要走最高的那条路。\n你敢不敢跟上来？",
    "看好了，我有一个新动作。\n昨天偷偷练会的。",
    "刚刚那次不算！\n都怪风突然变大了。",
    "我发现一个好地方。\n特别适合玩躲猫猫。",
    "谁先到屋顶谁就赢！\n最后一个要请吃东西。",
    "等一下，我鞋带还没系好。\n现在开始就不公平啦。",
    "准备好了吗？\n我要开始咯，一、二、三！",
};

static const char *const REST_LINES[] = {
    "呼……我只休息一下下。\n你要在旁边陪着我哦。",
    "今天玩得也太久了吧。\n我的腿都开始抗议啦。",
    "先坐一会儿。\n真的就只坐一会儿。",
    "有动静就轻轻叫醒我。\n太大声会把梦吓跑的。",
    "我闭上眼睛了。\n可还是能听见你哦。",
    "这里暖暖的，好舒服呀。\n我都不想起来了。",
    "五分钟后一定要叫我。\n不可以让我睡过头。",
    "我要梦见一大堆好吃的。\n醒来以后分你一点。",
    "你先帮我看着。\n我很快就会回来啦。",
    "不许趁我睡着偷偷跑掉。\n拉钩了就不能反悔。",
    "醒来以后继续比赛。\n这一次我还是会赢。",
    "嘘，先别说话。\n让风再吹一小会儿。",
};

static const char *const CARE_LINES[] = {
    "你今天是不是有点累？\n我们慢一点也没关系。",
    "记得喝点水呀。\n我会帮你看着这里的。",
    "肩膀是不是酸酸的？\n起来伸个懒腰吧。",
    "你已经做了好多事啦。\n剩下的可以慢慢来。",
    "今天要是不太顺利。\n也不代表你不厉害哦。",
    "你不想说也没关系。\n我陪你安静一会儿。",
    "外面冷的话要穿暖一点。\n别让自己着凉啦。",
    "肚子饿了就先吃东西。\n事情等一下也不会跑掉。",
    "眼睛累了就看看远处。\n我也陪你一起看。",
    "心里堵堵的时候。\n先慢慢呼一口气吧。",
    "今天有没有好好吃饭？\n不可以只顾着忙哦。",
    "做得不够完美也没事。\n我还是站在你这边。",
    "被别人误会一定很难受。\n至少我愿意听你说。",
    "今天辛苦啦。\n现在可以放松一点了。",
    "如果你想哭就哭吧。\n我保证不会笑你的。",
    "你对别人已经很好了。\n也要对自己好一点呀。",
    "有些答案要慢慢找。\n先休息一下也不会丢。",
    "别一直皱着眉头啦。\n额头也会累的。",
    "你不用一直勇敢。\n累的时候可以靠一下。",
    "不管今天发生了什么。\n你平安回来就好啦。",
};

static const char *const THOUGHT_LINES[] = {
    "影子一直跟着我。\n那它会不会也怕黑呀？",
    "远处的东西看起来很小。\n靠近后会变得不一样吗？",
    "人为什么会忘记呢？\n是不是脑袋装不下那么多事？",
    "如果我把今天记得很久，\n今天是不是就没有结束？",
    "勇敢是不是明明很害怕，\n还是愿意再往前走一步？",
    "长大到底是什么呀？\n是懂得更多，还是哭得更少？",
    "同一场雨落在每个人身上，\n大家听见的声音会一样吗？",
    "如果没人看见我做了好事，\n那它还是一件好事吧？",
    "有些路走错了才会发现，\n原来旁边也有好看的风景。",
    "你说，名字到底是什么？\n没有名字的云也还是云呀。",
    "昨天的我遇见今天的我，\n我们会成为好朋友吗？",
    "开心的时候过得特别快。\n时间是不是也会偷偷跑步？",
    "我有时会害怕做不好。\n可是不做，就永远学不会了。",
    "一个人安静不一定是孤单。\n也可能是在听心里的声音。",
    "如果愿望没有马上实现，\n是不是它还在来的路上？",
    "我不太喜欢说再见。\n但这样才会期待下次吧？",
    "我每天都会变一点点。\n改变也不用一下完成吧？",
    "我以前以为强大就是不哭。\n现在觉得哭完还能走也很厉害。",
    "我们看到的是同一种蓝吗？\n也许每个人的天空都不一样。",
    "世界明明这么大。\n重要的人为什么总能遇见呢？",
};

static const char *const AFFECTION_LOW_LINES[] = {
    "你又来看我啦。\n那就一起待一会儿吧。",
    "我还没有很了解你。\n不过你看起来不像坏人。",
    "先说好，我可不怕生。\n只是还没想好说什么。",
    "你不用一直盯着我啦。\n我又不会偷偷跑掉。",
    "这里是你的地盘吗？\n我还要再观察一下。",
    "我记住你的声音了。\n下次应该不会认错。",
    "你今天也会回来吗？\n我只是随便问问。",
    "我们才刚认识不久。\n慢慢来就好啦。",
    "你可以坐在旁边。\n但是不许笑我发呆。",
    "我还不知道怎么叫你。\n先叫你那个人好了。",
};

static const char *const AFFECTION_FAMILIAR_LINES[] = {
    "我现在一听见动静。\n就知道是你来啦。",
    "今天有件好玩的事。\n我一直留着想告诉你。",
    "你不在的时候。\n这里好像会安静一点。",
    "我已经记住你的习惯啦。\n虽然还没有全部记住。",
    "跟你说话不用想太久。\n想到什么就能说什么。",
    "你今天来得有点晚哦。\n我才没有一直等。",
    "我们已经算朋友了吧？\n不许说还不算。",
    "刚才看到好看的云。\n第一个就想叫你来看。",
    "有你在旁边的时候。\n连发呆都不会无聊。",
    "下次遇见好玩的事。\n我们还要一起去哦。",
};

static const char *const AFFECTION_HIGH_LINES[] = {
    "你终于来啦！\n我一眼就认出你了。",
    "今天最开心的事情。\n就是你又回来找我。",
    "不管去了多远。\n最后要记得回来哦。",
    "我有好多话只想告诉你。\n别人问我也不说。",
    "你累的时候就靠近一点。\n这次换我陪着你。",
    "我最喜欢和你待在一起。\n就算什么都不做也喜欢。",
    "你不开心的话。\n可以把一点难过分给我。",
    "以后还有好多天。\n我们可以慢慢一起度过。",
    "我当然会记得你。\n你可是很重要的人。",
    "拉钩吧，不管发生什么。\n我们都不要把对方忘掉。",
};

#define DIALOGUE_BANK(lines) { \
    lines, sizeof(lines) / sizeof((lines)[0]) \
}

static const dialogue_bank_t IDLE_DAWN_DIALOGUE = DIALOGUE_BANK(IDLE_DAWN_LINES);
static const dialogue_bank_t IDLE_DAY_DIALOGUE = DIALOGUE_BANK(IDLE_DAY_LINES);
static const dialogue_bank_t IDLE_DUSK_DIALOGUE = DIALOGUE_BANK(IDLE_DUSK_LINES);
static const dialogue_bank_t IDLE_NIGHT_DIALOGUE = DIALOGUE_BANK(IDLE_NIGHT_LINES);
static const dialogue_bank_t HAPPY_DIALOGUE = DIALOGUE_BANK(HAPPY_LINES);
static const dialogue_bank_t SAD_DIALOGUE = DIALOGUE_BANK(SAD_LINES);
static const dialogue_bank_t COOL_DIALOGUE = DIALOGUE_BANK(COOL_LINES);
static const dialogue_bank_t FEED_DIALOGUE = DIALOGUE_BANK(FEED_LINES);
static const dialogue_bank_t PLAY_DIALOGUE = DIALOGUE_BANK(PLAY_LINES);
static const dialogue_bank_t REST_DIALOGUE = DIALOGUE_BANK(REST_LINES);
static const dialogue_bank_t CARE_DIALOGUE = DIALOGUE_BANK(CARE_LINES);
static const dialogue_bank_t AFFECTION_LOW_DIALOGUE = DIALOGUE_BANK(AFFECTION_LOW_LINES);
static const dialogue_bank_t AFFECTION_FAMILIAR_DIALOGUE = DIALOGUE_BANK(AFFECTION_FAMILIAR_LINES);
static const dialogue_bank_t AFFECTION_HIGH_DIALOGUE = DIALOGUE_BANK(AFFECTION_HIGH_LINES);

static pet_state_t s_state;
static uint8_t s_frame_buffers[2][PET_FRAME_BYTES] __attribute__((aligned(4)));
static lv_image_dsc_t s_frame_images[2];
static uint8_t s_frame_buffer_index;
static tinfl_decompressor s_frame_decompressor;
static lv_obj_t *s_screen;
static lv_obj_t *s_pet_panel;
static lv_obj_t *s_image;
static lv_obj_t *s_stats;
static lv_obj_t *s_affection_value;
static lv_obj_t *s_battery;
static lv_obj_t *s_dialogue;
static lv_obj_t *s_affection_hearts[5];
static lv_obj_t *s_action_panels[PET_ACTION_COUNT];
static lv_timer_t *s_action_timer;
static lv_timer_t *s_frame_timer;
static lv_timer_t *s_status_timer;
static pet_action_t s_running_action;
static pet_asset_t s_current_asset;
static int s_frame_index;
static int s_action_stage;
static bool s_action_running;
static uint16_t s_last_dialogue = UINT16_MAX;
static const dialogue_bank_t *s_last_dialogue_bank;
static uint8_t s_last_thought = UINT8_MAX;
static uint8_t s_actions_since_save;
static uint8_t s_clock_hour;
static uint8_t s_clock_minute;
static int s_current_period = -1;

#define PET_SAVE_MAGIC 0x50455432u
#define PET_SAVE_MAGIC_LEGACY 0x50455431u

typedef struct {
    uint32_t magic;
    pet_state_t state;
} pet_save_t;

typedef struct {
    uint8_t energy;
    uint8_t mood;
    uint8_t hunger;
    pet_action_t selected;
    uint8_t level;
    uint8_t xp;
    uint8_t combo;
    pet_action_t last_action;
    pet_action_t mission_action;
    uint8_t mission_progress;
    uint8_t mission_target;
} pet_state_legacy_t;

typedef struct {
    uint32_t magic;
    pet_state_legacy_t state;
} pet_save_legacy_t;

static void say_random(const dialogue_bank_t *bank)
{
    uint16_t choice;
    do {
        choice = (uint16_t)(esp_random() % bank->count);
    } while (bank == s_last_dialogue_bank && choice == s_last_dialogue && bank->count > 1);
    s_last_dialogue_bank = bank;
    s_last_dialogue = choice;
    lv_label_set_text(s_dialogue, bank->lines[choice]);
}

static void say_random_thought(void)
{
    const uint8_t count = sizeof(THOUGHT_LINES) / sizeof(THOUGHT_LINES[0]);
    uint8_t choice;
    do {
        choice = (uint8_t)(esp_random() % count);
    } while (choice == s_last_thought && count > 1);
    s_last_thought = choice;
    lv_label_set_text(s_dialogue, THOUGHT_LINES[choice]);
}

static void save_state(void)
{
    nvs_handle_t handle;
    if (nvs_open("pet", NVS_READWRITE, &handle) != ESP_OK) return;
    pet_save_t save = { .magic = PET_SAVE_MAGIC, .state = s_state };
    if (nvs_set_blob(handle, "state", &save, sizeof(save)) == ESP_OK) {
        nvs_commit(handle);
        s_actions_since_save = 0;
    }
    nvs_close(handle);
}

static bool load_state(void)
{
    nvs_handle_t handle;
    if (nvs_open("pet", NVS_READONLY, &handle) != ESP_OK) return false;
    uint8_t buffer[sizeof(pet_save_legacy_t)];
    size_t size = sizeof(buffer);
    esp_err_t result = nvs_get_blob(handle, "state", buffer, &size);
    nvs_close(handle);
    if (result != ESP_OK || size < sizeof(uint32_t)) return false;

    uint32_t magic;
    memcpy(&magic, buffer, sizeof(magic));
    if (magic == PET_SAVE_MAGIC && size == sizeof(pet_save_t)) {
        pet_save_t save;
        memcpy(&save, buffer, sizeof(save));
        if (save.state.selected >= PET_ACTION_COUNT ||
            save.state.last_action > PET_ACTION_COUNT ||
            save.state.affection > 100) {
            return false;
        }
        s_state = save.state;
        return true;
    }
    if (magic == PET_SAVE_MAGIC_LEGACY && size == sizeof(pet_save_legacy_t)) {
        pet_save_legacy_t legacy;
        memcpy(&legacy, buffer, sizeof(legacy));
        if (legacy.state.selected >= PET_ACTION_COUNT ||
            legacy.state.last_action > PET_ACTION_COUNT ||
            legacy.state.level < 1 || legacy.state.level > 20) {
            return false;
        }
        pet_state_init(&s_state);
        s_state.energy = legacy.state.energy;
        s_state.mood = legacy.state.mood;
        s_state.hunger = legacy.state.hunger;
        s_state.selected = legacy.state.selected;
        s_state.last_action = legacy.state.last_action;
        unsigned affection = 15u + (legacy.state.level - 1u) * 5u +
                              legacy.state.xp / 4u;
        s_state.affection = (uint8_t)(affection > 100 ? 100 : affection);
        save_state();
        return true;
    }
    return false;
}

static uint16_t read_le16(const uint8_t *data)
{
    return (uint16_t)(data[0] | ((uint16_t)data[1] << 8));
}

static uint32_t read_le32(const uint8_t *data)
{
    return (uint32_t)data[0] |
           ((uint32_t)data[1] << 8) |
           ((uint32_t)data[2] << 16) |
           ((uint32_t)data[3] << 24);
}

static uint16_t asset_frame_count(pet_asset_t asset)
{
    return read_le16(ASSET_DATA[asset].start + 8);
}

static uint16_t asset_frame_duration(pet_asset_t asset, uint16_t frame)
{
    uint16_t count = asset_frame_count(asset);
    const uint8_t *durations = ASSET_DATA[asset].start +
                               PET_ASSET_HEADER_BYTES + 4 * (count + 1);
    return read_le16(durations + 2 * frame);
}

static bool validate_asset(pet_asset_t asset)
{
    const embedded_asset_t *embedded = &ASSET_DATA[asset];
    size_t size = (size_t)(embedded->end - embedded->start);
    if (size < PET_ASSET_HEADER_BYTES ||
        memcmp(embedded->start, "PZF1", 4) != 0 ||
        read_le16(embedded->start + 4) != PET_FRAME_WIDTH ||
        read_le16(embedded->start + 6) != PET_FRAME_HEIGHT) {
        return false;
    }
    uint16_t count = asset_frame_count(asset);
    size_t tables_end = PET_ASSET_HEADER_BYTES + 4 * (count + 1) + 2 * count;
    if (count == 0 || tables_end > size) return false;
    const uint8_t *offsets = embedded->start + PET_ASSET_HEADER_BYTES;
    return read_le32(offsets) >= tables_end &&
           read_le32(offsets + 4 * count) <= size;
}

static bool decode_frame(pet_asset_t asset, uint16_t frame)
{
    const embedded_asset_t *embedded = &ASSET_DATA[asset];
    uint16_t count = asset_frame_count(asset);
    frame %= count;
    const uint8_t *offsets = embedded->start + PET_ASSET_HEADER_BYTES;
    uint32_t start = read_le32(offsets + 4 * frame);
    uint32_t end = read_le32(offsets + 4 * (frame + 1));
    size_t asset_size = (size_t)(embedded->end - embedded->start);
    if (start >= end || end > asset_size) return false;

    uint8_t next_buffer = s_frame_buffer_index ^ 1;
    size_t input_size = end - start;
    size_t output_size = PET_FRAME_BYTES;
    tinfl_init(&s_frame_decompressor);
    tinfl_status status = tinfl_decompress(
        &s_frame_decompressor,
        embedded->start + start,
        &input_size,
        s_frame_buffers[next_buffer],
        s_frame_buffers[next_buffer],
        &output_size,
        TINFL_FLAG_PARSE_ZLIB_HEADER |
            TINFL_FLAG_USING_NON_WRAPPING_OUTPUT_BUF);
    if (status != TINFL_STATUS_DONE || output_size != PET_FRAME_BYTES) {
        ESP_LOGE(TAG, "asset %d frame %u decode failed: status=%d size=%u",
                 asset, frame, status, (unsigned)output_size);
        return false;
    }

    s_frame_buffer_index = next_buffer;
    s_frame_index = frame;
    lv_image_dsc_t *image = &s_frame_images[next_buffer];
    image->data = s_frame_buffers[next_buffer];
    lv_image_set_src(s_image, image);
    if (s_frame_timer) {
        lv_timer_set_period(s_frame_timer, asset_frame_duration(asset, frame));
    }
    return true;
}

static void set_animation(pet_asset_t asset)
{
    s_current_asset = asset;
    decode_frame(asset, 0);
    lv_obj_align(s_image, LV_ALIGN_CENTER, 0, 0);
}

static ui_time_period_t period_for_hour(uint8_t hour)
{
    if (hour >= 5 && hour < 8) return UI_TIME_DAWN;
    if (hour >= 8 && hour < 17) return UI_TIME_DAY;
    if (hour >= 17 && hour < 20) return UI_TIME_DUSK;
    return UI_TIME_NIGHT;
}

static const dialogue_bank_t *idle_dialogue_for_period(void)
{
    switch (period_for_hour(s_clock_hour)) {
    case UI_TIME_DAWN:
        return &IDLE_DAWN_DIALOGUE;
    case UI_TIME_DAY:
        return &IDLE_DAY_DIALOGUE;
    case UI_TIME_DUSK:
        return &IDLE_DUSK_DIALOGUE;
    case UI_TIME_NIGHT:
    default:
        return &IDLE_NIGHT_DIALOGUE;
    }
}

static const dialogue_bank_t *affection_dialogue(void)
{
    switch (pet_state_affection_stage(&s_state)) {
    case PET_AFFECTION_LOW:
        return &AFFECTION_LOW_DIALOGUE;
    case PET_AFFECTION_FAMILIAR:
        return &AFFECTION_FAMILIAR_DIALOGUE;
    case PET_AFFECTION_HIGH:
    default:
        return (esp_random() & 1)
            ? &AFFECTION_HIGH_DIALOGUE
            : &AFFECTION_FAMILIAR_DIALOGUE;
    }
}

static void apply_time_theme(bool force)
{
    ui_time_period_t period = period_for_hour(s_clock_hour);
    if (!force && s_current_period == period) return;
    s_current_period = period;
    ui_pixel_screen_set_period(s_screen, period);

    static const uint32_t image_tint[] = {
        [UI_TIME_DAWN] = 0xFFB06A,
        [UI_TIME_DAY] = 0xFFFFFF,
        [UI_TIME_DUSK] = 0xB76583,
        [UI_TIME_NIGHT] = 0x315A8C,
    };
    static const lv_opa_t image_tint_opa[] = {
        [UI_TIME_DAWN] = LV_OPA_20,
        [UI_TIME_DAY] = LV_OPA_TRANSP,
        [UI_TIME_DUSK] = LV_OPA_20,
        [UI_TIME_NIGHT] = LV_OPA_30,
    };
    static const uint32_t panel_color[] = {
        [UI_TIME_DAWN] = 0x30252A,
        [UI_TIME_DAY] = 0x20212A,
        [UI_TIME_DUSK] = 0x211C2B,
        [UI_TIME_NIGHT] = 0x111722,
    };
    lv_obj_set_style_bg_color(s_pet_panel, lv_color_hex(panel_color[period]), 0);
    lv_obj_set_style_image_recolor(s_image, lv_color_hex(image_tint[period]), 0);
    lv_obj_set_style_image_recolor_opa(s_image, image_tint_opa[period], 0);
}

static void clock_init_from_build(void)
{
    /* Offline fallback: the build host supplies the initial local time. */
    const char *build_time = __TIME__;
    s_clock_hour = (uint8_t)((build_time[0] - '0') * 10 + (build_time[1] - '0'));
    s_clock_minute = (uint8_t)((build_time[3] - '0') * 10 + (build_time[4] - '0'));
}

static void clock_advance_minute(void)
{
    if (++s_clock_minute >= 60) {
        s_clock_minute = 0;
        s_clock_hour = (uint8_t)((s_clock_hour + 1) % 24);
    }
    apply_time_theme(false);
}

static void frame_timer_cb(lv_timer_t *timer)
{
    uint16_t count = asset_frame_count(s_current_asset);
    if (!decode_frame(s_current_asset, (uint16_t)(s_frame_index + 1) % count)) {
        lv_timer_pause(timer);
    }
}

static void settle_pet(void)
{
    pet_mood_t mood = pet_state_mood(&s_state);
    pet_affection_stage_t affection_stage =
        pet_state_affection_stage(&s_state);
    uint32_t roll = esp_random() % 100;
    uint32_t ambient_roll = esp_random() % 100;
    if (mood == PET_MOOD_SAD) {
        set_animation(PET_ASSET_SAD);
        say_random(&SAD_DIALOGUE);
    } else if (mood == PET_MOOD_COOL && roll < 15) {
        set_animation(PET_ASSET_COOL);
        say_random(&COOL_DIALOGUE);
    } else if (affection_stage == PET_AFFECTION_HIGH &&
               ambient_roll < 12) {
        set_animation(PET_ASSET_IDLE);
        say_random(&CARE_DIALOGUE);
    } else if (affection_stage == PET_AFFECTION_FAMILIAR &&
               ambient_roll < 5) {
        set_animation(PET_ASSET_IDLE);
        say_random(&CARE_DIALOGUE);
    } else if (ambient_roll < 18) {
        set_animation(PET_ASSET_IDLE);
        say_random(affection_dialogue());
    } else if (ambient_roll < 30) {
        set_animation(PET_ASSET_IDLE);
        say_random_thought();
    } else if (s_state.mood >= 80 && roll < 65) {
        set_animation(PET_ASSET_HAPPY);
        say_random(&HAPPY_DIALOGUE);
    } else {
        set_animation(PET_ASSET_IDLE);
        say_random(idle_dialogue_for_period());
    }
}

static void set_heart_color(lv_obj_t *heart, uint32_t color)
{
    uint32_t child_count = lv_obj_get_child_count(heart);
    for (uint32_t i = 0; i < child_count; ++i) {
        lv_obj_set_style_bg_color(
            lv_obj_get_child(heart, i), lv_color_hex(color), 0);
    }
}

static lv_obj_t *heart_pixel(lv_obj_t *heart, int x, int y, int w, int h)
{
    lv_obj_t *pixel = lv_obj_create(heart);
    lv_obj_remove_flag(pixel, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_pos(pixel, x, y);
    lv_obj_set_size(pixel, w, h);
    lv_obj_set_style_radius(pixel, 0, 0);
    lv_obj_set_style_border_width(pixel, 0, 0);
    lv_obj_set_style_pad_all(pixel, 0, 0);
    return pixel;
}

static lv_obj_t *create_pixel_heart(lv_obj_t *parent, int x, int y)
{
    lv_obj_t *heart = lv_obj_create(parent);
    lv_obj_remove_flag(heart, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_pos(heart, x, y);
    lv_obj_set_size(heart, 13, 11);
    lv_obj_set_style_bg_opa(heart, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(heart, 0, 0);
    lv_obj_set_style_pad_all(heart, 0, 0);
    heart_pixel(heart, 2, 0, 4, 2);
    heart_pixel(heart, 7, 0, 4, 2);
    heart_pixel(heart, 0, 2, 13, 3);
    heart_pixel(heart, 2, 5, 9, 2);
    heart_pixel(heart, 4, 7, 5, 2);
    heart_pixel(heart, 6, 9, 1, 2);
    return heart;
}

static void refresh_ui(void)
{
    unsigned filled_hearts = (s_state.affection + 19u) / 20u;
    for (unsigned i = 0; i < 5; ++i) {
        set_heart_color(s_affection_hearts[i],
                        i < filled_hearts ? 0xE43B5A : 0xB7C3C8);
    }
    lv_label_set_text_fmt(s_affection_value, "好感 %u", s_state.affection);
    lv_label_set_text_fmt(s_stats, "体力%u  心情%u  饱腹%u",
                          s_state.energy, s_state.mood, s_state.hunger);
    for (int i = 0; i < PET_ACTION_COUNT; ++i) {
        ui_pixel_set_selected(s_action_panels[i], i == s_state.selected, true);
    }
}

static void show_affection_stage_message(void)
{
    if (pet_state_affection_stage(&s_state) == PET_AFFECTION_HIGH) {
        lv_label_set_text(s_dialogue,
            "你对我来说很重要啦。\n以后也要一直在一起哦。");
    } else {
        lv_label_set_text(s_dialogue,
            "我们已经算朋友了吧？\n以后也要常来找我哦。");
    }
}

static void action_timer_cb(lv_timer_t *timer)
{
    if (s_running_action == PET_ACTION_FEED && s_action_stage == 0) {
        s_action_stage = 1;
        set_animation(PET_ASSET_HAPPY);
        lv_timer_set_period(timer, 2600);
        lv_timer_reset(timer);
        return;
    }
    if (s_running_action == PET_ACTION_PLAY && s_action_stage == 0) {
        s_action_stage = 1;
        set_animation((esp_random() % 100 < 15) ? PET_ASSET_COOL : PET_ASSET_HAPPY);
        lv_timer_set_period(timer, 1800);
        lv_timer_reset(timer);
        return;
    }

    lv_timer_pause(timer);
    s_action_running = false;
    settle_pet();
    ESP_LOGI(TAG, "action done, free=%lu largest=%lu",
             (unsigned long)esp_get_free_heap_size(),
             (unsigned long)heap_caps_get_largest_free_block(MALLOC_CAP_8BIT));
}

static void start_action(void)
{
    s_running_action = s_state.selected;
    s_action_stage = 0;
    s_action_running = true;
    pet_action_result_t result = pet_state_apply(&s_state);

    uint32_t period;
    switch (s_running_action) {
    case PET_ACTION_FEED:
        set_animation(PET_ASSET_SURPRISED);
        say_random(&FEED_DIALOGUE);
        period = 1600;
        break;
    case PET_ACTION_PLAY:
        set_animation(PET_ASSET_PLAY);
        say_random(&PLAY_DIALOGUE);
        period = 3200;
        break;
    case PET_ACTION_REST:
    default:
        set_animation(PET_ASSET_REST);
        say_random(&REST_DIALOGUE);
        period = 4800;
        break;
    }

    bool important = result.affection_stage_changed;
    if (result.affection_stage_changed) {
        show_affection_stage_message();
    } else if (esp_random() % 100 < 20) {
        pet_event_t event = (pet_event_t)(esp_random() % PET_EVENT_COUNT);
        bool event_stage_changed = pet_state_apply_event(&s_state, event);
        important = important || event_stage_changed;
        static const char *const EVENT_LINES[PET_EVENT_COUNT] = {
            "哇，地上有个补给箱！\n里面居然全是好吃的。",
            "呀，怎么突然下雨啦！\n快找个地方躲一躲。",
            "我发现一条没人知道的小路！\n嘘，这是我们的秘密。",
            "太阳出来啦！\n整座城市一下变亮了。",
        };
        if (event_stage_changed) show_affection_stage_message();
        else lv_label_set_text(s_dialogue, EVENT_LINES[event]);
    }

    s_actions_since_save++;
    if (important || s_actions_since_save >= 5) save_state();
    refresh_ui();
    lv_timer_set_period(s_action_timer, period);
    lv_timer_reset(s_action_timer);
    lv_timer_resume(s_action_timer);
}

static void status_timer_cb(lv_timer_t *timer)
{
    static unsigned seconds;
    (void)timer;
    int soc = bsp_battery_soc();
    if (soc >= 0) lv_label_set_text_fmt(s_battery, "%d%%", soc);
    else lv_label_set_text(s_battery, "--%");

    if (++seconds % 60 == 0) {
        clock_advance_minute();
        pet_state_decay(&s_state);
        refresh_ui();
        if (!s_action_running) settle_pet();
    }
}

void pet_app_enter(void)
{
    clock_init_from_build();
    s_current_period = -1;
    pet_state_init(&s_state);
    load_state();
    for (int i = 0; i < PET_ASSET_COUNT; ++i) {
        if (!validate_asset((pet_asset_t)i)) {
            ESP_LOGE(TAG, "asset %d has an invalid compressed frame table", i);
            return;
        }
    }
    s_frame_buffer_index = 0;
    s_frame_timer = NULL;
    for (int i = 0; i < 2; ++i) {
        lv_image_dsc_t *image = &s_frame_images[i];
        memset(image, 0, sizeof(*image));
        image->header.cf = LV_COLOR_FORMAT_RGB565;
        image->header.w = PET_FRAME_WIDTH;
        image->header.h = PET_FRAME_HEIGHT;
        image->header.stride = PET_FRAME_WIDTH * 2;
        image->data = s_frame_buffers[i];
        image->data_size = PET_FRAME_BYTES;
    }

    s_screen = ui_pixel_screen_create(NULL);
    lv_obj_t *dialogue_panel = ui_pixel_panel_create(s_screen, 8, 8, 224, 58, UI_PAPER);
    lv_obj_set_style_pad_all(dialogue_panel, 1, 0);
    s_dialogue = ui_pixel_label(dialogue_panel, "", &lv_font_pet_zh_14, UI_INK);
    lv_obj_set_width(s_dialogue, 210);
    lv_label_set_long_mode(s_dialogue, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_align(s_dialogue, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_line_space(s_dialogue, 1, 0);
    lv_obj_center(s_dialogue);

    s_battery = ui_pixel_label(s_screen, "--%", &lv_font_montserrat_14, UI_INK);
    lv_obj_set_pos(s_battery, 190, 70);

    s_pet_panel = ui_pixel_panel_create(s_screen, 55, 68, 130, 135, 0x20212A);
    lv_obj_set_style_pad_all(s_pet_panel, 0, 0);
    s_image = lv_image_create(s_pet_panel);
    apply_time_theme(true);
    settle_pet();

    lv_obj_t *stats_panel = ui_pixel_panel_create(s_screen, 18, 208, 204, 42, UI_PAPER);
    lv_obj_set_style_pad_all(stats_panel, 0, 0);
    for (int i = 0; i < 5; ++i) {
        s_affection_hearts[i] = create_pixel_heart(
            stats_panel, 12 + i * 15, 7);
    }
    s_affection_value = ui_pixel_label(
        stats_panel, "", &lv_font_pet_zh_14, UI_INK);
    lv_obj_set_pos(s_affection_value, 96, 4);
    s_stats = ui_pixel_label(stats_panel, "", &lv_font_pet_zh_14, UI_INK);
    lv_obj_set_style_text_align(s_stats, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(s_stats, 190);
    lv_obj_set_pos(s_stats, 3, 21);

    for (int i = 0; i < PET_ACTION_COUNT; ++i) {
        s_action_panels[i] = ui_pixel_panel_create(s_screen, 8 + i * 78, 255, 68, 30, UI_PAPER);
        lv_obj_t *label = ui_pixel_label(s_action_panels[i], ACTION_NAMES[i],
                                         &lv_font_pet_zh_14, UI_INK);
        lv_obj_center(label);
    }

    refresh_ui();
    s_action_timer = lv_timer_create(action_timer_cb, 1000, NULL);
    lv_timer_pause(s_action_timer);
    s_frame_timer = lv_timer_create(frame_timer_cb, 150, NULL);
    s_status_timer = lv_timer_create(status_timer_cb, 1000, NULL);
    status_timer_cb(NULL);
    lv_screen_load(s_screen);
}

void pet_app_key(bsp_btn_t btn, bsp_btn_ev_t event)
{
    if (event == BSP_BTN_LONG && (btn == BSP_BTN_UP || btn == BSP_BTN_DOWN)) {
        s_clock_hour = (uint8_t)((s_clock_hour + (btn == BSP_BTN_UP ? 1 : 23)) % 24);
        apply_time_theme(true);
        lv_label_set_text_fmt(s_dialogue, "现在是%02u:%02u。\n天空也换好颜色啦！",
                              s_clock_hour, s_clock_minute);
        return;
    }
    if (event != BSP_BTN_CLICK) return;
    if (btn == BSP_BTN_UP) {
        pet_state_move(&s_state, -1);
        refresh_ui();
    } else if (btn == BSP_BTN_DOWN) {
        pet_state_move(&s_state, 1);
        refresh_ui();
    } else if (btn == BSP_BTN_OK) {
        start_action();
    }
}
