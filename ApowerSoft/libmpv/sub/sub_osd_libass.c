/*
 * This file is part of mpv.
 *
 * mpv is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * mpv is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with mpv.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "mpv-config.h"

#include "mpv_talloc.h"
#include "misc/bstr.h"
#include "common/common.h"
#include "common/msg.h"
#include "osd.h"
#include "osd_state.h"

static const char osd_font_pfb[] =
// Generated from E:/tools/msys2/mingw32/mpv/sub/osd_font.otf

"OTTO\000\013\000\200\000\003\0000CFF \327\275G\005\000\000\005\334\000\000\n"
"\313FFTM\202\225\214P\000\000\020\310\000\000\000\034GDEF\000'\000(\000\000\020\250\000\000\000\036OS/22\232C\017\000\000\001 \000\000\000\140cmap\301f\245-\000\000\004X\000\000\001bhead\014\3015\276\000\000\000\274\000\000\0006hhea\007Z\006\202\000\000\000\364\000\000\000\044hmtx~\140\0062\000\000\020\344\000\000\000\210maxp\000\"P\000\000\000\001\030\000\000\000\006name\255\202\263\354\000\000\001\200\000\000\002\326post\377e\0002\000\000\005\274\000\000\000 \000\001\000\000\000\001\000\000=N\370\375_\017<\365\000\013\003\350\000\000\000\000\320\033\356\032\000\000\000\000\332\027\003\237\377\377\377\366\003p\003*\000\000\000\010\000\002\000\000\000\000\000\000\000\001\000\000\003\350\3778\000\000\007\271\377\377\377n\003p\000\001\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\"\000\000P\000\000\"\000\000\000\003\003\310\001\220\000\005\000\010\002\212\002X\000\000\000K\002\212\002X\000\000\001^\0002\000\334\000\000\000\000\005\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000PfEd\000\100\340\001\341\025\003 \3778\000Z\003\350\000\310\000\000\000\001\000\000\000\000\000\000\000\000\000 \000 \000\001\000\000\000\016\000\256\000\001\000\000\000\000\000\000\000\027\0000\000\001\000\000\000\000\000\001\000\017\000h\000\001\000\000\000\000\000\002\000\007\000\210\000\001\000\000\000\000\000\003\000\"\000\326\000\001\000\000\000\000\000\004\000\017\001\031\000\001\000\000\000\000\000\005\000<\001\243\000\001\000\000\000\000\000\006\000\027\002\020\000\003\000\001\004\t\000\000\000.\000\000\000\003\000\001\004\t\000\001\000\036\000H\000\003\000\001\004\t\000\002\000\016\000x\000\003\000\001\004\t\000\003\000D\000\220\000\003\000\001\004\t\000\004\000\036\000\371\000\003\000\001\004\t\000\005\000x\001)\000\003\000\001\004\t\000\006\000.\001\340\000T\000h\000i\000s\000 \000i\000s\000 \000g\000e\000n\000e\000r\000a\000t\000e\000d\000 \000f\000i\000l\000e\000.\000\000This is generated file.\000\000m\000p\000v\000-\000o\000s\000d\000-\000s\000y\000m\000b\000o\000l\000s\000\000mpv-osd-symbols\000\000R\000e\000g\000u\000l\000a\000r\000\000Regular\000\0001\000.\0000\0000\0000\000;\000P\000f\000E\000d\000;\000m\000p\000v\000-\000o\000s\000d\000-\000s\000y\000m\000b\000o\000l\000s\000-\000R\000e\000g\000u\000l\000a\000r\000\0001.000;PfEd;mpv-osd-symbols-Regular\000\000m\000p\000v\000-\000o\000s\000d\000-\000s\000y\000m\000b\000o\000l\000s\000\000mpv-osd-symbols\000\000V\000e\000r\000s\000i\000o\000n\000 \0001\000.\0000\0000\0000\000;\000P\000S\000 \0000\0000\0001\000.\0000\0000\0000\000;\000h\000o\000t\000c\000o\000n\000v\000 \0001\000.\0000\000.\0007\0000\000;\000m\000a\000k\000e\000o\000t\000f\000.\000l\000i\000b\0002\000.\0005\000.\0005\0008\0003\0002\0009\000\000Version 1.000;PS 001.000;hotconv 1.0.70;makeotf.lib2.5.58329\000\000m\000p\000v\000-\000o\000s\000d\000-\000s\000y\000m\000b\000o\000l\000s\000-\000R\000e\000g\000u\000l\000a\000r\000\000mpv-osd-symbols-Regular\000\000\000\000\000\000\003\000\000\000\003\000\000\000\034\000\001\000\000\000\000\000\\\000\003\000\001\000\000\000\034\000\004\000\100\000\000\000\014\000\010\000\002\000\004\340\013\340\023\341\001\341\016\341\025\377\377\000\000\340\001\340\020\341\001\341\004\341\020\377\377 \000\037\374\037\017\037\015\037\014\000\001\000\000\000\000\000\000\000\000\000\000\000\000\000\000\001\006\000\000\001\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\003\000\000\000\000\000\000\377b\0002\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\001\000\004\004\000\001\001\001\030mpv-osd-symbols-Regular\000\001\002\000\001\000.\370\017\000\370<\001\370=\002\370>\003\370\077\004\373\031\014\003\212\201\372\004\371\276\005\034\001\317\017\034\000\000\020\034\002\022\021\034\000-\034\t\314\022\000%\002\000\001\000\010\000\017\000\026\000\035\000\044\000+\0002\0009\000\100\000G\000N\000U\000\\\000c\000j\000q\000x\000\177\000\206\000\215\000\224\000\233\000\242\000\251\000\260\000\267\000\276\000\305\000\314\000\323\000\332\000\341\000\350\000\377\001\026\001%\001+uniE001uniE002uniE003uniE004uniE005uniE006uniE007uniE008uniE009uniE00AuniE00BuniE010uniE011uniE012uniE013uniE101uniE104uniE105uniE106uniE107uniE108uniE109uniE10AuniE10BuniE10CuniE10DuniE10EuniE110uniE111uniE112uniE113uniE114uniE115This is generated file.mpv-osd-symbols Regularmpv-osd-symbolsNormal\000\000\000\001\207\001\210\001\211\001\212\001\213\001\214\001\215\001\216\001\217\001\220\001\221\001\222\001\223\001\224\001\225\001\226\001\227\001\230\001\231\001\232\001\233\001\234\001\235\001\236\001\237\001\240\001\241\001\242\001\243\001\244\001\245\001\246\001\247\000\"\002\000\001\000\003\000\024\000:\000P\000g\000~\001\017\0012\001\276\001\324\002{\002\302\002\337\002\365\003\021\003(\003\077\003h\003\234\003\351\004T\004\232\004\334\005O\005j\005\214\005\341\006o\006\203\006\257\006\301\006\347\007<\007r\366\016\371\264w\001\367\\\370\013\003\370\323\370\044\025 \n"
"\016\213\371\264\001\367\\\367*\367*\367*\003\367\362\371\264\025\373*\375\264\367*\006\367\300\371\264\025\373*\375\264\367*\006\016\213\371d\001\367\002\371-\003\371\233\371d\025\375-\375d\371-\006\016\371\264w\001\273\371\221\003\370;\371\264\025!\n"
"\370\032\371\264\025!\n"
"\016\371\264w\001\352\371\221\003\370j\370\044\025 \n"
"\371\221\370\044\025 \n"
"\016\201\226\367W\367\077\255\252\370\005\224\022\237\224\367W\367Ai\255\213\314\213\367A\367W\225\023\231\100\370B\371\276\025\373v\373L\373L\373v\373v\367L\373L\367v\367v\367L\367L\367v\037\367v\373L\367L\373v\036\023R\200j\373\140\025\367W\314\373W\007\373\203\004\023D\000J\264\006\023B\200<\3676\270\241\327\373/\367\210\367p\246m\373\214\373s\005\373\202\251\025J\373W\314\007\371\265I\025\373W\314\367W\006\374\005\373\201\025\373WJ\367W\007\016\201\333\371(\333\001\237\333\003\370B\201\025\"\n"
"\333\004\373J\373(\367(\367J\367J\367(\367(\367J\037\016\201\333\367>\3674\333\3674\345\333\022\237\333\367\003\3674v\3674u\3674\367\004\333\023\372\200\370B\201\025\367v\367L\367L\367v\367v\373L\367L\373v\373v\373L\373L\373v\037\373v\367L\373L\367v\036\333\004\373J\373(\367(\367J\367J\367(\367(\367J\367J\367(\373(\373J\373J\373(\373(\373J\037\370.\004#\n"
"\023\375\200\373\037\373\204\025\267\257\257\267\267g\257__gg_\037_\257g\267\036\367\251\026#\n"
"\016\371\140\225\025\213\371\243\373\345\373~\373j\213\213\373\317\367i\213\005\016\367\220\273\326\275\326\273\356w\001\367\256\273\327\275\325\273\003\371\271\370\013\025\275\373w\007\207\242\202\240\177\235\010\3674\3674h\256\3734\3734\005y\230v\224t\217\010\367vY\373v\007t\207v\202x\177\010\3733\3673hh\3673\3733\005~y\202u\207t\010\373uY\367u\006\217t\224u\230y\010\3733\3733\256h\3673\3673\005\236\177\240\202\242\207\010\373v\275\367v\007\242\217\240\224\235\230\010\3674\3734\256\256\3734\3674\005\227\235\224\240\217\242\010\373(\100\025T^\270\302\302\270\270\302\302\270^TT^^T\037\016\201\367\210\367\340\301\367\014\321\001\237\343\371\030\343\003\370B\201\025\"\n"
"\371\n"
"\004jp\246\254\254\246\246\254\254\246pjjppj\037\374\026\004\373F\373\044\325\347\347\367\044\325\367F\367F\367\044A//\373\044A\373F\037\016\264\213\345\370\140\345\001\327\367%\003\367\226\371\024\025\373J\375\024\367J\345f\370\140\260\006\016\264\213\371\024\001\337\367:\003\367\216\371\024\025\373:\375\024\367:\006\016\264\213\345\370\140\345\001\367\005\367%\003\367\005\345\025f1\367J\371\024\373J1\260\006\016\264\367\223\367\026\001\361\367\026\003\367|\370\025\025\373\026\373\026\367\026\006\016\371\264w\001\367r\370\354\003\371\312\370\044\025\374\354\370\044\213\375\264\005\016\213\371\264\213w\022\216\367\017\023\140\367\022\371\264\025\373\017\006\023\240\375\264\367\017\007\370\014\371\264\025!\n"
"\370\014\371\264\025!\n"
"\016\213\371\264\213w\022\371\207\367\017\023\140\216\026\370\013\370\044\374\013\370\044\005\023\240\370\014\375\264\025\370\013\370\044\374\013\370\044\005\370\207\375\264\025\371\264\373\017\375\264\007\016\213\371\035\001\213\372\004\003\372\004\370\020\025\367\024\373l\367!\373k\373S\373\203\373\015\373(\036\213 \262S\330^\215\207\213\206\213\206\213JT^9\203\243q\247}\254\213\363\213\344\274\220\320\300\200\301\205\306\213\010\367\200\367W\367\025\367*\037\016\213\307\271\275\276\275\367\327\307\022\213\307\262\367n\373\077\367Y\237\367\021\264\367\004\262\302\340\307\023\375\340\307\004\044\n"
"\307\374\234\025%\n"
"\374\213\367Y\025\373nY\367n\006\367\077\275\025\373\021Y\367\021\006\367\271\275\025\373\220Y\367\220\006\023B\200\374JX\025\373YY\367Y\006\367\276\275\025\023A\300\373\217Y\367\217\006\351\275\025TY\302\006\016\333\307\367I\3673\367H\307\001\213\307\367j\367\340\367j\307\003\367 \004\044\n"
"\307\374\234\025%\n"
"\373\204\370\140\025\367H\373H\213\367H\005\373.\373\014\025\373\340\3733\367\340\006\373\306\373\015\025\373H\367H\213\373H\005\016\333\307\3670\367e\367/\307\001\213\307\307\366\305\367\312\305\366\307\307\003\367 \004\044\n"
"\307\374\234\025%\n"
"\374\345\367\230\025 \367\002\213\373p\005\371\024\026\213\367p \373\002\005Q\364\025\373\312\373e\367\312\006\016\034\006\224\367\206\367\214\001\370\"\305\025&\n"
"\371\"\367\256\025\206\213\206\212\210\207\010\373\022\373\022\373\022\367\022\005\203\222\200\213\203\204\204\203\213\200\222\204\010\367\022\373\022\373\021\373\022\005\203\203\213\177\223\203\222\204\230\213\222\222\010\367\022\367\022\367\022\373\022\005\222\204\227\213\222\222\222\223\213\226\204\222\010\373\022\367\022\367\022\367\022\005\223\223\213\227\203\223\207\217\206\214\206\213\010\016\034\006\224\367\206\367\214\001\370\357\272\003\371\036\370\002\025'\n"
"\373\220\373\310\025&\n"
"\016\034\006\224\367\206\367\214\001\370\357\272\316\272\003\371\220\370\002\025(\n"
"\373\006\026'\n"
"\373\220\373\310\025&\n"
"\016\034\006\224\367\206\367\214\367\203w\001\370\357\272\316\272\315\272\003\372\001\370\002\025\213\367\026V\367\015 \367\004\010es\005\360\"\275\373\005\213\373\015\213\373\020Y\373\004&\044\010\261s\005\366\367\004\300\367\015\213\367\026\010\373\005\026(\n"
"\373\006\026'\n"
"\373\220\373\310\025&\n"
"\016\034\006\224\274\352\355\367\214\001\370\303\272\316\272\272\346\003\371d\370\002\025(\n"
"\373\006\026'\n"
"\373d\373\310\025&\n"
"\371S\370*\025\217\007\213\230\206\225\202\222\202\223\201\217~\213\177\213\200\207\202\203\202\204\207\201\213~\010\213\207\254\374.\005\214}\217\204\222\213\222\213\217\222\214\231\010\255\373!\025\213\230\206\226\203\224\202\225\200\217~\213\177\213\200\207\202\201\202\202\207\200\213~\213}\217\200\224\202\224\201\226\207\227\213\010\230\213\226\217\224\225\223\224\220\226\213\231\010\016\371\264w\001\303\370\354\003\371\044\026\213\371\264\374\354\374\044\005\016\367 \004\044\n"
"\307\374\234\025%\n"
"\374\345\367\230\025 \367\002\213\373p\005\371\024\026\213\367p \373\002\005Q\364\025\373\312\373e\367\312\006\016\370o\263\367\024\001\370\224\263\025\367\024\374\224\373\024\007\016\367\335\265\313\370\224\367\024\001\213\313\371\024\313\003\371\224\265\025\371T\375\224\375T\007\371T\313\025\375\024\370\224\371\024\006\016\370\357\265\313\367T\313\213\367\024\367\024\367\024\022\213\313\367\024\313\367\224\313\367\024\313\023\337\371\224\367\276\025\370T\374\324\007\023\277\373\224\373T\374T\370\324\007\023\337\367\224\007\367\024\313\025\373\024\006\023\277\367\024\373\324\367\024\370T\007\373T\374\224\025\374T\367\224\370T\006\016\370\273\3713\263\025\213\356\373\201\367\201\367\201\367\201\213\356+\213\373\204\373\201\373\201\367\201(\213\213(\367\201\373\201\373\201\373\204\213+\356\213\367\201\367\201\367\204\373\201\005\016\372\004\024\367\271\025\201\225\371\024\225\321\225\321\225\006\036\n"
"\003\177\014\t\213\014\n"
"\213\014\013\314\n"
"\314\371s\014\014\314\013\314\340\014\015\034\000-\023\000\t\001\001\n"
"\0246Hfn\200\242\306\374\013\370\044\213\375\264\005\013\374\013\374\044\370\013\374\044\005\013\367v\367L\367L\367v\367v\373L\367L\373v\373v\373L\373L\373v\373v\367L\373L\367v\037\013\267\257\257\267\267g\257__gg__\257g\267\037\013m\251m\251\036\371\214\006\251\251\251\251\037\370\234\007\251m\251m\036\375\214\006mmmm\037\013\370\234\371\214\374\234\007\013\213\370\374\373\206\373L\373.\213\213\373\214\367-\213\005\013\213\341h\335E\332\010et\005\313C\253A\213=\213<kAKD\010\261t\005\321\332\256\336\213\340\010\013\213\367\002_\3612\350\010es\005\3353\265.\213'\213(a-92\010\261t\005\344\352\267\361\213\367\000\010\013\000\000\001\000\000\000\014\000\000\000\026\000\000\000\002\000\001\000\001\000!\000\001\000\004\000\000\000\002\000\000\000\000\000\000\000\001\000\000\000\000\330b\232\226\000\000\000\000\320\033\356\032\000\000\000\000\332\027\003\237\001\220\000\000\003p\000\310\003p\000\310\003p\000n\003p\0000\003p\000_\003p\000\024\003p\000\024\003p\000\024\003p\000\245\003p\0009\003p\000\024\001N\000L\001N\000T\001N\000L\001N\000f\003p\000\336\003p\000\003\003p\000\003\003p\000\000\003p\000\000\003p\000\000\003p\000\000\007\271\000\002\007\271\000\002\007\271\000\002\007\271\000\002\007\271\000\002\003p\0008\003p\000\000\003\000\000\000\002n\000\000\003\200\000\000\003L\377\377"
;

#include "sub/ass_mp.h"
#include "options/options.h"


#define ASS_USE_OSD_FONT "{\\fnmpv-osd-symbols}"

static void append_ass(struct ass_state *ass, struct mp_osd_res *res,
                       ASS_Image **img_list, bool *changed);

void osd_init_backend(struct osd_state *osd)
{
}

static void create_ass_renderer(struct osd_state *osd, struct ass_state *ass)
{
    if (ass->render)
        return;

    ass->log = mp_log_new(NULL, osd->log, "libass");
    ass->library = mp_ass_init(osd->global, ass->log);
    ass_add_font(ass->library, "mpv-osd-symbols", (void *)osd_font_pfb,
                 sizeof(osd_font_pfb) - 1);

    ass->render = ass_renderer_init(ass->library);
    if (!ass->render)
        abort();

    mp_ass_configure_fonts(ass->render, osd->opts->osd_style,
                           osd->global, ass->log);
    ass_set_pixel_aspect(ass->render, 1.0);
}

static void destroy_ass_renderer(struct ass_state *ass)
{
    if (ass->track)
        ass_free_track(ass->track);
    ass->track = NULL;
    if (ass->render)
        ass_renderer_done(ass->render);
    ass->render = NULL;
    if (ass->library)
        ass_library_done(ass->library);
    ass->library = NULL;
    talloc_free(ass->log);
    ass->log = NULL;
}

static void destroy_external(struct osd_external *ext)
{
    destroy_ass_renderer(&ext->ass);
    talloc_free(ext);
}

void osd_destroy_backend(struct osd_state *osd)
{
    for (int n = 0; n < MAX_OSD_PARTS; n++) {
        struct osd_object *obj = osd->objs[n];
        destroy_ass_renderer(&obj->ass);
        for (int i = 0; i < obj->num_externals; i++)
            destroy_external(obj->externals[i]);
        obj->num_externals = 0;
    }
}

static void update_playres(struct ass_state *ass, struct mp_osd_res *vo_res)
{
    ASS_Track *track = ass->track;
    int old_res_x = track->PlayResX;
    int old_res_y = track->PlayResY;

    ass->vo_res = *vo_res;

    double aspect = 1.0 * vo_res->w / MPMAX(vo_res->h, 1);
    if (vo_res->display_par > 0)
        aspect = aspect / vo_res->display_par;

    track->PlayResY = ass->res_y ? ass->res_y : MP_ASS_FONT_PLAYRESY;
    track->PlayResX = ass->res_x ? ass->res_x : track->PlayResY * aspect;

    // Force libass to clear its internal cache - it doesn't check for
    // PlayRes changes itself.
    if (old_res_x != track->PlayResX || old_res_y != track->PlayResY)
        ass_set_frame_size(ass->render, 1, 1);
}

static void create_ass_track(struct osd_state *osd, struct osd_object *obj,
                             struct ass_state *ass)
{
    create_ass_renderer(osd, ass);

    ASS_Track *track = ass->track;
    if (!track)
        track = ass->track = ass_new_track(ass->library);

    track->track_type = TRACK_TYPE_ASS;
    track->Timer = 100.;
    track->WrapStyle = 1; // end-of-line wrapping instead of smart wrapping
    track->Kerning = true;
    track->ScaledBorderAndShadow = true;

    update_playres(ass, &obj->vo_res);
}

static int find_style(ASS_Track *track, const char *name, int def)
{
    for (int n = 0; n < track->n_styles; n++) {
        if (track->styles[n].Name && strcmp(track->styles[n].Name, name) == 0)
            return n;
    }
    return def;
}

// Find a given style, or add it if it's missing.
static ASS_Style *get_style(struct ass_state *ass, char *name)
{
    ASS_Track *track = ass->track;
    if (!track)
        return NULL;

    int sid = find_style(track, name, -1);
    if (sid >= 0)
        return &track->styles[sid];

    sid = ass_alloc_style(track);
    ASS_Style *style = &track->styles[sid];
    style->Name = strdup(name);
    // Set to neutral base direction, as opposed to VSFilter LTR default
    style->Encoding = -1;
    return style;
}

static ASS_Event *add_osd_ass_event(ASS_Track *track, const char *style,
                                    const char *text)
{
    int n = ass_alloc_event(track);
    ASS_Event *event = track->events + n;
    event->Start = 0;
    event->Duration = 100;
    event->Style = find_style(track, style, 0);
    event->ReadOrder = n;
    assert(event->Text == NULL);
    if (text)
        event->Text = strdup(text);
    return event;
}

static void clear_ass(struct ass_state *ass)
{
    if (ass->track)
        ass_flush_events(ass->track);
}

void osd_get_function_sym(char *buffer, size_t buffer_size, int osd_function)
{
    // 0xFF is never valid UTF-8, so we can use it to escape OSD symbols.
    // (Same trick as OSD_ASS_0/OSD_ASS_1.)
    snprintf(buffer, buffer_size, "\xFF%c", osd_function);
}

static void mangle_ass(bstr *dst, const char *in)
{
    const char *start = in;
    bool escape_ass = true;
    while (*in) {
        // As used by osd_get_function_sym().
        if (in[0] == '\xFF' && in[1]) {
            bstr_xappend(NULL, dst, bstr0(ASS_USE_OSD_FONT));
            mp_append_utf8_bstr(NULL, dst, OSD_CODEPOINTS + in[1]);
            bstr_xappend(NULL, dst, bstr0("{\\r}"));
            in += 2;
            continue;
        }
        if (*in == OSD_ASS_0[0] || *in == OSD_ASS_1[0]) {
            escape_ass = *in == OSD_ASS_1[0];
            in += 1;
            continue;
        }
        if (escape_ass && *in == '{')
            bstr_xappend(NULL, dst, bstr0("\\"));
        // Libass will strip leading whitespace
        if (in[0] == ' ' && (in == start || in[-1] == '\n')) {
            bstr_xappend(NULL, dst, bstr0("\\h"));
            in += 1;
            continue;
        }
        bstr_xappend(NULL, dst, (bstr){(char *)in, 1});
        // Break ASS escapes with U+2060 WORD JOINER
        if (escape_ass && *in == '\\')
            mp_append_utf8_bstr(NULL, dst, 0x2060);
        in++;
    }
}

static ASS_Event *add_osd_ass_event_escaped(ASS_Track *track, const char *style,
                                            const char *text)
{
    bstr buf = {0};
    mangle_ass(&buf, text);
    ASS_Event *e = add_osd_ass_event(track, style, buf.start);
    talloc_free(buf.start);
    return e;
}

static ASS_Style *prepare_osd_ass(struct osd_state *osd, struct osd_object *obj)
{
    struct mp_osd_render_opts *opts = osd->opts;

    create_ass_track(osd, obj, &obj->ass);

    struct osd_style_opts font = *opts->osd_style;
    font.font_size *= opts->osd_scale;

    double playresy = obj->ass.track->PlayResY;
    // Compensate for libass and mp_ass_set_style scaling the font etc.
    if (!opts->osd_scale_by_window)
        playresy *= 720.0 / obj->vo_res.h;

    ASS_Style *style = get_style(&obj->ass, "OSD");
    mp_ass_set_style(style, playresy, &font);
    return style;
}

static void update_osd_text(struct osd_state *osd, struct osd_object *obj)
{

    if (!obj->text[0])
        return;

    prepare_osd_ass(osd, obj);
    add_osd_ass_event_escaped(obj->ass.track, "OSD", obj->text);
}

void osd_get_text_size(struct osd_state *osd, int *out_screen_h, int *out_font_h)
{
    pthread_mutex_lock(&osd->lock);
    struct osd_object *obj = osd->objs[OSDTYPE_OSD];
    ASS_Style *style = prepare_osd_ass(osd, obj);
    *out_screen_h = obj->ass.track->PlayResY - style->MarginV;
    *out_font_h = style->FontSize;
    pthread_mutex_unlock(&osd->lock);
}

// align: -1 .. +1
// frame: size of the containing area
// obj: size of the object that should be positioned inside the area
// margin: min. distance from object to frame (as long as -1 <= align <= +1)
static float get_align(float align, float frame, float obj, float margin)
{
    frame -= margin * 2;
    return margin + frame / 2 - obj / 2 + (frame - obj) / 2 * align;
}

struct ass_draw {
    int scale;
    char *text;
};

static void ass_draw_start(struct ass_draw *d)
{
    d->scale = MPMAX(d->scale, 1);
    d->text = talloc_asprintf_append(d->text, "{\\p%d}", d->scale);
}

static void ass_draw_stop(struct ass_draw *d)
{
    d->text = talloc_strdup_append(d->text, "{\\p0}");
}

static void ass_draw_c(struct ass_draw *d, float x, float y)
{
    int ix = round(x * (1 << (d->scale - 1)));
    int iy = round(y * (1 << (d->scale - 1)));
    d->text = talloc_asprintf_append(d->text, " %d %d", ix, iy);
}

static void ass_draw_append(struct ass_draw *d, const char *t)
{
    d->text = talloc_strdup_append(d->text, t);
}

static void ass_draw_move_to(struct ass_draw *d, float x, float y)
{
    ass_draw_append(d, " m");
    ass_draw_c(d, x, y);
}

static void ass_draw_line_to(struct ass_draw *d, float x, float y)
{
    ass_draw_append(d, " l");
    ass_draw_c(d, x, y);
}

static void ass_draw_rect_ccw(struct ass_draw *d, float x0, float y0,
                              float x1, float y1)
{
    ass_draw_move_to(d, x0, y0);
    ass_draw_line_to(d, x0, y1);
    ass_draw_line_to(d, x1, y1);
    ass_draw_line_to(d, x1, y0);
}

static void ass_draw_rect_cw(struct ass_draw *d, float x0, float y0,
                             float x1, float y1)
{
    ass_draw_move_to(d, x0, y0);
    ass_draw_line_to(d, x1, y0);
    ass_draw_line_to(d, x1, y1);
    ass_draw_line_to(d, x0, y1);
}

static void ass_draw_reset(struct ass_draw *d)
{
    talloc_free(d->text);
    d->text = NULL;
}

static void get_osd_bar_box(struct osd_state *osd, struct osd_object *obj,
                            float *o_x, float *o_y, float *o_w, float *o_h,
                            float *o_border)
{
    struct mp_osd_render_opts *opts = osd->opts;

    create_ass_track(osd, obj, &obj->ass);
    ASS_Track *track = obj->ass.track;

    ASS_Style *style = get_style(&obj->ass, "progbar");
    if (!style) {
        *o_x = *o_y = *o_w = *o_h = *o_border = 0;
        return;
    }

    mp_ass_set_style(style, track->PlayResY, opts->osd_style);

    if (osd->opts->osd_style->back_color.a) {
        // override the default osd opaque-box into plain outline. Otherwise
        // the opaque box is not aligned with the bar (even without shadow),
        // and each bar ass event gets its own opaque box - breaking the bar.
        style->BackColour = MP_ASS_COLOR(opts->osd_style->shadow_color);
        style->BorderStyle = 1; // outline
    }

    *o_w = track->PlayResX * (opts->osd_bar_w / 100.0);
    *o_h = track->PlayResY * (opts->osd_bar_h / 100.0);

    float base_size = 0.03125;
    style->Outline *= *o_h / track->PlayResY / base_size;
    // So that the chapter marks have space between them
    style->Outline = MPMIN(style->Outline, *o_h / 5.2);
    // So that the border is not 0
    style->Outline = MPMAX(style->Outline, *o_h / 32.0);
    // Rendering with shadow is broken (because there's more than one shape)
    style->Shadow = 0;

    style->Alignment = 5;

    *o_border = style->Outline;

    *o_x = get_align(opts->osd_bar_align_x, track->PlayResX, *o_w, *o_border);
    *o_y = get_align(opts->osd_bar_align_y, track->PlayResY, *o_h, *o_border);
}

static void update_progbar(struct osd_state *osd, struct osd_object *obj)
{
    if (obj->progbar_state.type < 0)
        return;

    float px, py, width, height, border;
    get_osd_bar_box(osd, obj, &px, &py, &width, &height, &border);

    ASS_Track *track = obj->ass.track;

    float sx = px - border * 2 - height / 4; // includes additional spacing
    float sy = py + height / 2;

    bstr buf = bstr0(talloc_asprintf(NULL, "{\\an6\\pos(%f,%f)}", sx, sy));

    if (obj->progbar_state.type == 0 || obj->progbar_state.type >= 256) {
        // no sym
    } else if (obj->progbar_state.type >= 32) {
        mp_append_utf8_bstr(NULL, &buf, obj->progbar_state.type);
    } else {
        bstr_xappend(NULL, &buf, bstr0(ASS_USE_OSD_FONT));
        mp_append_utf8_bstr(NULL, &buf, OSD_CODEPOINTS + obj->progbar_state.type);
        bstr_xappend(NULL, &buf, bstr0("{\\r}"));
    }

    add_osd_ass_event(track, "progbar", buf.start);
    talloc_free(buf.start);

    struct ass_draw *d = &(struct ass_draw) { .scale = 4 };

    if (osd->opts->osd_style->back_color.a) {
        // the bar style always ignores the --osd-back-color config - it messes
        // up the bar. draw an artificial box at the original back color.
        struct m_color bc = osd->opts->osd_style->back_color;
        d->text = talloc_asprintf_append(d->text,
            "{\\pos(%f,%f)\\bord0\\1a&H%02X\\1c&H%02X%02X%02X&}",
             px, py, 255 - bc.a, (int)bc.b, (int)bc.g, (int)bc.r);

        ass_draw_start(d);
        ass_draw_rect_cw(d, -border, -border, width + border, height + border);
        ass_draw_stop(d);
        add_osd_ass_event(track, "progbar", d->text);
        ass_draw_reset(d);
    }

    // filled area
    d->text = talloc_asprintf_append(d->text, "{\\bord0\\pos(%f,%f)}", px, py);
    ass_draw_start(d);
    float pos = obj->progbar_state.value * width - border / 2;
    ass_draw_rect_cw(d, 0, 0, pos, height);
    ass_draw_stop(d);
    add_osd_ass_event(track, "progbar", d->text);
    ass_draw_reset(d);

    // position marker
    d->text = talloc_asprintf_append(d->text, "{\\bord%f\\pos(%f,%f)}",
                                     border / 2, px, py);
    ass_draw_start(d);
    ass_draw_move_to(d, pos + border / 2, 0);
    ass_draw_line_to(d, pos + border / 2, height);
    ass_draw_stop(d);
    add_osd_ass_event(track, "progbar", d->text);
    ass_draw_reset(d);

    d->text = talloc_asprintf_append(d->text, "{\\pos(%f,%f)}", px, py);
    ass_draw_start(d);

    // the box
    ass_draw_rect_cw(d, -border, -border, width + border, height + border);

    // the "hole"
    ass_draw_rect_ccw(d, 0, 0, width, height);

    // chapter marks
    for (int n = 0; n < obj->progbar_state.num_stops; n++) {
        float s = obj->progbar_state.stops[n] * width;
        float dent = border * 1.3;

        if (s > dent && s < width - dent) {
            ass_draw_move_to(d, s + dent, 0);
            ass_draw_line_to(d, s,        dent);
            ass_draw_line_to(d, s - dent, 0);

            ass_draw_move_to(d, s - dent, height);
            ass_draw_line_to(d, s,        height - dent);
            ass_draw_line_to(d, s + dent, height);
        }
    }

    ass_draw_stop(d);
    add_osd_ass_event(track, "progbar", d->text);
    ass_draw_reset(d);
}

static void update_osd(struct osd_state *osd, struct osd_object *obj)
{
    obj->osd_changed = false;
    clear_ass(&obj->ass);
    update_osd_text(osd, obj);
    update_progbar(osd, obj);
}

static void update_external(struct osd_state *osd, struct osd_object *obj,
                            struct osd_external *ext)
{
    bstr t = bstr0(ext->ov.data);
    ext->ass.res_x = ext->ov.res_x;
    ext->ass.res_y = ext->ov.res_y;
    create_ass_track(osd, obj, &ext->ass);

    clear_ass(&ext->ass);

    int resy = ext->ass.track->PlayResY;
    mp_ass_set_style(get_style(&ext->ass, "OSD"), resy, osd->opts->osd_style);

    // Some scripts will reference this style name with \r tags.
    const struct osd_style_opts *def = osd_style_conf.defaults;
    mp_ass_set_style(get_style(&ext->ass, "Default"), resy, def);

    while (t.len) {
        bstr line;
        bstr_split_tok(t, "\n", &line, &t);
        if (line.len) {
            char *tmp = bstrdup0(NULL, line);
            add_osd_ass_event(ext->ass.track, "OSD", tmp);
            talloc_free(tmp);
        }
    }
}

static int cmp_zorder(const void *pa, const void *pb)
{
    const struct osd_external *a = *(struct osd_external **)pa;
    const struct osd_external *b = *(struct osd_external **)pb;
    return a->ov.z == b->ov.z ? 0 : (a->ov.z > b->ov.z ? 1 : -1);
}

void osd_set_external(struct osd_state *osd, struct osd_external_ass *ov)
{
    pthread_mutex_lock(&osd->lock);
    struct osd_object *obj = osd->objs[OSDTYPE_EXTERNAL];
    bool zorder_changed = false;
    int index = -1;

    for (int n = 0; n < obj->num_externals; n++) {
        struct osd_external *e = obj->externals[n];
        if (e->ov.id == ov->id && e->ov.owner == ov->owner) {
            index = n;
            break;
        }
    }

    if (index < 0) {
        if (!ov->format)
            goto done;
        struct osd_external *new = talloc_zero(NULL, struct osd_external);
        new->ov.owner = ov->owner;
        new->ov.id = ov->id;
        MP_TARRAY_APPEND(obj, obj->externals, obj->num_externals, new);
        index = obj->num_externals - 1;
        zorder_changed = true;
    }

    struct osd_external *entry = obj->externals[index];

    if (!ov->format) {
        if (!entry->ov.hidden) {
            obj->changed = true;
            osd->want_redraw_notification = true;
        }
        destroy_external(entry);
        MP_TARRAY_REMOVE_AT(obj->externals, obj->num_externals, index);
        goto done;
    }

    entry->ov.format = ov->format;
    if (!entry->ov.data)
        entry->ov.data = talloc_strdup(entry, "");
    entry->ov.data[0] = '\0'; // reuse memory allocation
    entry->ov.data = talloc_strdup_append(entry->ov.data, ov->data);
    entry->ov.res_x = ov->res_x;
    entry->ov.res_y = ov->res_y;
    zorder_changed |= entry->ov.z != ov->z;
    entry->ov.z = ov->z;
    entry->ov.hidden = ov->hidden;

    update_external(osd, obj, entry);

    if (!entry->ov.hidden) {
        obj->changed = true;
        osd->want_redraw_notification = true;
    }

    if (zorder_changed) {
        qsort(obj->externals, obj->num_externals, sizeof(obj->externals[0]),
              cmp_zorder);
    }

    if (ov->out_rc) {
        struct mp_osd_res vo_res = entry->ass.vo_res;
        // Defined fallback if VO has not drawn this yet
        if (vo_res.w < 1 || vo_res.h < 1) {
            vo_res = (struct mp_osd_res){
                .w = entry->ov.res_x,
                .h = entry->ov.res_y,
                .display_par = 1,
            };
            // According to osd-overlay command description.
            if (vo_res.w < 1)
                vo_res.w = 1280;
            if (vo_res.h < 1)
                vo_res.h = 720;
        }

        ASS_Image *img_list = NULL;
        append_ass(&entry->ass, &vo_res, &img_list, NULL);

        mp_ass_get_bb(img_list, entry->ass.track, &vo_res, ov->out_rc);
    }

done:
    pthread_mutex_unlock(&osd->lock);
}

void osd_set_external_remove_owner(struct osd_state *osd, void *owner)
{
    pthread_mutex_lock(&osd->lock);
    struct osd_object *obj = osd->objs[OSDTYPE_EXTERNAL];
    for (int n = obj->num_externals - 1; n >= 0; n--) {
        struct osd_external *e = obj->externals[n];
        if (e->ov.owner == owner) {
            destroy_external(e);
            MP_TARRAY_REMOVE_AT(obj->externals, obj->num_externals, n);
            obj->changed = true;
            osd->want_redraw_notification = true;
        }
    }
    pthread_mutex_unlock(&osd->lock);
}

static void append_ass(struct ass_state *ass, struct mp_osd_res *res,
                       ASS_Image **img_list, bool *changed)
{
    if (!ass->render || !ass->track) {
        *img_list = NULL;
        return;
    }

    update_playres(ass, res);

    ass_set_frame_size(ass->render, res->w, res->h);
    ass_set_pixel_aspect(ass->render, res->display_par);

    int ass_changed;
    *img_list = ass_render_frame(ass->render, ass->track, 0, &ass_changed);

    ass->changed |= ass_changed;

    if (changed) {
        *changed |= ass->changed;
        ass->changed = false;
    }
}

struct sub_bitmaps *osd_object_get_bitmaps(struct osd_state *osd,
                                           struct osd_object *obj, int format)
{
    if (obj->type == OSDTYPE_OSD && obj->osd_changed)
        update_osd(osd, obj);

    if (!obj->ass_packer)
        obj->ass_packer = mp_ass_packer_alloc(obj);

    MP_TARRAY_GROW(obj, obj->ass_imgs, obj->num_externals + 1);

    append_ass(&obj->ass, &obj->vo_res, &obj->ass_imgs[0], &obj->changed);
    for (int n = 0; n < obj->num_externals; n++) {
        if (obj->externals[n]->ov.hidden) {
            update_playres(&obj->externals[n]->ass, &obj->vo_res);
            obj->ass_imgs[n + 1] = NULL;
        } else {
            append_ass(&obj->externals[n]->ass, &obj->vo_res,
                       &obj->ass_imgs[n + 1], &obj->changed);
        }
    }

    struct sub_bitmaps out_imgs = {0};
    mp_ass_packer_pack(obj->ass_packer, obj->ass_imgs, obj->num_externals + 1,
                       obj->changed, format, &out_imgs);

    obj->changed = false;

    return sub_bitmaps_copy(&obj->copy_cache, &out_imgs);
}
