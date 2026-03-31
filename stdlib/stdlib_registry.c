/*
 * X# Standard Library - Registry Implementation
 * ================================================
 * Registers ALL stdlib functions from every module with the VM.
 */

#include "stdlib_registry.h"

/* Include all module headers */
#include "math/math_lib.h"
#include "string/string_lib.h"
#include "collections/collections_lib.h"
#include "io/io_lib.h"
#include "graphics/graphics_lib.h"
#include "ai/ai_lib.h"
#include "net/net_lib.h"
#include "system/system_lib.h"
#include "audio/audio_lib.h"
#include "physics/physics_lib.h"
#include "crypto/crypto_lib.h"
#include "thread/thread_lib.h"
#include "time/time_lib.h"
#include "regex/regex_lib.h"
#include "fs/fs_lib.h"

void xs_register_all_stdlib(VM *vm) {
    /* ===== Math Module (40 functions) ===== */
    vm_register_native(vm, "Math.PI",          xs_math_PI);
    vm_register_native(vm, "Math.E",           xs_math_E);
    vm_register_native(vm, "Math.TAU",         xs_math_TAU);
    vm_register_native(vm, "Math.INF",         xs_math_INF);
    vm_register_native(vm, "Math.NAN",         xs_math_NAN_VAL);
    vm_register_native(vm, "Math.abs",         xs_math_abs);
    vm_register_native(vm, "Math.ceil",        xs_math_ceil);
    vm_register_native(vm, "Math.floor",       xs_math_floor);
    vm_register_native(vm, "Math.round",       xs_math_round);
    vm_register_native(vm, "Math.sign",        xs_math_sign);
    vm_register_native(vm, "Math.fract",       xs_math_fract);
    vm_register_native(vm, "Math.sqrt",        xs_math_sqrt);
    vm_register_native(vm, "Math.cbrt",        xs_math_cbrt);
    vm_register_native(vm, "Math.pow",         xs_math_pow);
    vm_register_native(vm, "Math.exp",         xs_math_exp);
    vm_register_native(vm, "Math.log",         xs_math_log);
    vm_register_native(vm, "Math.log2",        xs_math_log2);
    vm_register_native(vm, "Math.log10",       xs_math_log10);
    vm_register_native(vm, "Math.sin",         xs_math_sin);
    vm_register_native(vm, "Math.cos",         xs_math_cos);
    vm_register_native(vm, "Math.tan",         xs_math_tan);
    vm_register_native(vm, "Math.asin",        xs_math_asin);
    vm_register_native(vm, "Math.acos",        xs_math_acos);
    vm_register_native(vm, "Math.atan",        xs_math_atan);
    vm_register_native(vm, "Math.atan2",       xs_math_atan2);
    vm_register_native(vm, "Math.sinh",        xs_math_sinh);
    vm_register_native(vm, "Math.cosh",        xs_math_cosh);
    vm_register_native(vm, "Math.tanh",        xs_math_tanh);
    vm_register_native(vm, "Math.min",         xs_math_min);
    vm_register_native(vm, "Math.max",         xs_math_max);
    vm_register_native(vm, "Math.clamp",       xs_math_clamp);
    vm_register_native(vm, "Math.lerp",        xs_math_lerp);
    vm_register_native(vm, "Math.inverseLerp", xs_math_inverseLerp);
    vm_register_native(vm, "Math.remap",       xs_math_remap);
    vm_register_native(vm, "Math.step",        xs_math_step);
    vm_register_native(vm, "Math.smoothstep",  xs_math_smoothstep);
    vm_register_native(vm, "Math.random",      xs_math_random);
    vm_register_native(vm, "Math.randomRange", xs_math_randomRange);
    vm_register_native(vm, "Math.randomInt",   xs_math_randomInt);
    vm_register_native(vm, "Math.seedRandom",  xs_math_seedRandom);
    vm_register_native(vm, "Math.degToRad",    xs_math_degToRad);
    vm_register_native(vm, "Math.radToDeg",    xs_math_radToDeg);

    /* ===== String Module (25 functions) ===== */
    vm_register_native(vm, "String.length",        xs_string_length);
    vm_register_native(vm, "String.charAt",        xs_string_charAt);
    vm_register_native(vm, "String.substring",     xs_string_substring);
    vm_register_native(vm, "String.indexOf",       xs_string_indexOf);
    vm_register_native(vm, "String.lastIndexOf",   xs_string_lastIndexOf);
    vm_register_native(vm, "String.contains",      xs_string_contains);
    vm_register_native(vm, "String.startsWith",    xs_string_startsWith);
    vm_register_native(vm, "String.endsWith",      xs_string_endsWith);
    vm_register_native(vm, "String.toUpper",       xs_string_toUpper);
    vm_register_native(vm, "String.toLower",       xs_string_toLower);
    vm_register_native(vm, "String.trim",          xs_string_trim);
    vm_register_native(vm, "String.trimStart",     xs_string_trimStart);
    vm_register_native(vm, "String.trimEnd",       xs_string_trimEnd);
    vm_register_native(vm, "String.split",         xs_string_split);
    vm_register_native(vm, "String.join",          xs_string_join);
    vm_register_native(vm, "String.replace",       xs_string_replace);
    vm_register_native(vm, "String.replaceAll",    xs_string_replaceAll);
    vm_register_native(vm, "String.repeat",        xs_string_repeat);
    vm_register_native(vm, "String.reverse",       xs_string_reverse);
    vm_register_native(vm, "String.padStart",      xs_string_padStart);
    vm_register_native(vm, "String.padEnd",        xs_string_padEnd);
    vm_register_native(vm, "String.format",        xs_string_format);
    vm_register_native(vm, "String.concat",        xs_string_concat);
    vm_register_native(vm, "String.codePointAt",   xs_string_codePointAt);
    vm_register_native(vm, "String.fromCodePoint", xs_string_fromCodePoint);

    /* ===== Collections Module (32 functions) ===== */
    vm_register_native(vm, "Arsenal.push",      xs_col_push);
    vm_register_native(vm, "Arsenal.pop",       xs_col_pop);
    vm_register_native(vm, "Arsenal.shift",     xs_col_shift);
    vm_register_native(vm, "Arsenal.unshift",   xs_col_unshift);
    vm_register_native(vm, "Arsenal.slice",     xs_col_slice);
    vm_register_native(vm, "Arsenal.splice",    xs_col_splice);
    vm_register_native(vm, "Arsenal.concat",    xs_col_concat);
    vm_register_native(vm, "Arsenal.map",       xs_col_map);
    vm_register_native(vm, "Arsenal.filter",    xs_col_filter);
    vm_register_native(vm, "Arsenal.reduce",    xs_col_reduce);
    vm_register_native(vm, "Arsenal.forEach",   xs_col_forEach);
    vm_register_native(vm, "Arsenal.find",      xs_col_find);
    vm_register_native(vm, "Arsenal.findIndex", xs_col_findIndex);
    vm_register_native(vm, "Arsenal.sort",      xs_col_sort);
    vm_register_native(vm, "Arsenal.reverse",   xs_col_reverse);
    vm_register_native(vm, "Arsenal.includes",  xs_col_includes);
    vm_register_native(vm, "Arsenal.indexOf",   xs_col_indexOf);
    vm_register_native(vm, "Arsenal.flat",      xs_col_flat);
    vm_register_native(vm, "Arsenal.zip",       xs_col_zip);
    vm_register_native(vm, "Arsenal.enumerate", xs_col_enumerate);
    vm_register_native(vm, "Arsenal.range",     xs_col_range);
    vm_register_native(vm, "Arsenal.len",       xs_col_len);
    vm_register_native(vm, "Arsenal.fill",      xs_col_fill);
    vm_register_native(vm, "Arsenal.every",     xs_col_every);
    vm_register_native(vm, "Arsenal.some",      xs_col_some);
    vm_register_native(vm, "len",               xs_col_len);
    vm_register_native(vm, "HashMap.new",       xs_col_hashmap_new);
    vm_register_native(vm, "HashMap.set",       xs_col_hashmap_set);
    vm_register_native(vm, "HashMap.get",       xs_col_hashmap_get);
    vm_register_native(vm, "HashMap.delete",    xs_col_hashmap_delete);
    vm_register_native(vm, "HashMap.has",       xs_col_hashmap_has);
    vm_register_native(vm, "HashMap.keys",      xs_col_hashmap_keys);
    vm_register_native(vm, "HashMap.values",    xs_col_hashmap_values);

    /* ===== IO Module (20 functions) ===== */
    vm_register_native(vm, "engrave",       xs_io_engrave);
    vm_register_native(vm, "engraveLn",     xs_io_engraveLn);
    vm_register_native(vm, "IO.engrave",    xs_io_engrave);
    vm_register_native(vm, "IO.engraveLn",  xs_io_engraveLn);
    vm_register_native(vm, "IO.readLine",   xs_io_readLine);
    vm_register_native(vm, "IO.readChar",   xs_io_readChar);
    vm_register_native(vm, "IO.readFile",   xs_io_readFile);
    vm_register_native(vm, "IO.writeFile",  xs_io_writeFile);
    vm_register_native(vm, "IO.appendFile", xs_io_appendFile);
    vm_register_native(vm, "IO.fileExists", xs_io_fileExists);
    vm_register_native(vm, "IO.deleteFile", xs_io_deleteFile);
    vm_register_native(vm, "IO.copyFile",   xs_io_copyFile);
    vm_register_native(vm, "IO.moveFile",   xs_io_moveFile);
    vm_register_native(vm, "IO.fileSize",   xs_io_fileSize);
    vm_register_native(vm, "IO.listDir",    xs_io_listDir);
    vm_register_native(vm, "IO.mkdir",      xs_io_mkdir);
    vm_register_native(vm, "IO.rmdir",      xs_io_rmdir);
    vm_register_native(vm, "IO.cwd",        xs_io_cwd);
    vm_register_native(vm, "IO.chdir",      xs_io_chdir);
    vm_register_native(vm, "IO.isDir",      xs_io_isDir);

    /* ===== Graphics Module (19 functions) ===== */
    vm_register_native(vm, "Gfx.createWindow",    xs_gfx_createWindow);
    vm_register_native(vm, "Gfx.destroyWindow",   xs_gfx_destroyWindow);
    vm_register_native(vm, "Gfx.clearScreen",     xs_gfx_clearScreen);
    vm_register_native(vm, "Gfx.setColor",        xs_gfx_setColor);
    vm_register_native(vm, "Gfx.drawPixel",       xs_gfx_drawPixel);
    vm_register_native(vm, "Gfx.drawLine",        xs_gfx_drawLine);
    vm_register_native(vm, "Gfx.drawRect",        xs_gfx_drawRect);
    vm_register_native(vm, "Gfx.fillRect",        xs_gfx_fillRect);
    vm_register_native(vm, "Gfx.drawCircle",      xs_gfx_drawCircle);
    vm_register_native(vm, "Gfx.fillCircle",      xs_gfx_fillCircle);
    vm_register_native(vm, "Gfx.drawTriangle",    xs_gfx_drawTriangle);
    vm_register_native(vm, "Gfx.fillTriangle",    xs_gfx_fillTriangle);
    vm_register_native(vm, "Gfx.drawText",        xs_gfx_drawText);
    vm_register_native(vm, "Gfx.loadImage",       xs_gfx_loadImage);
    vm_register_native(vm, "Gfx.drawImage",       xs_gfx_drawImage);
    vm_register_native(vm, "Gfx.getScreenWidth",  xs_gfx_getScreenWidth);
    vm_register_native(vm, "Gfx.getScreenHeight", xs_gfx_getScreenHeight);
    vm_register_native(vm, "Gfx.pollEvents",      xs_gfx_pollEvents);
    vm_register_native(vm, "Gfx.swapBuffers",     xs_gfx_swapBuffers);

    /* ===== AI Module (17 functions) ===== */
    vm_register_native(vm, "AI.createNeuralNet",  xs_ai_createNeuralNet);
    vm_register_native(vm, "AI.addLayer",         xs_ai_addLayer);
    vm_register_native(vm, "AI.train",            xs_ai_train);
    vm_register_native(vm, "AI.predict",          xs_ai_predict);
    vm_register_native(vm, "AI.backpropagate",    xs_ai_backpropagate);
    vm_register_native(vm, "AI.setLearningRate",  xs_ai_setLearningRate);
    vm_register_native(vm, "AI.saveModel",        xs_ai_saveModel);
    vm_register_native(vm, "AI.loadModel",        xs_ai_loadModel);
    vm_register_native(vm, "AI.createMatrix",     xs_ai_createMatrix);
    vm_register_native(vm, "AI.matMul",           xs_ai_matMul);
    vm_register_native(vm, "AI.matAdd",           xs_ai_matAdd);
    vm_register_native(vm, "AI.matTranspose",     xs_ai_matTranspose);
    vm_register_native(vm, "AI.sigmoid",          xs_ai_sigmoid);
    vm_register_native(vm, "AI.relu",             xs_ai_relu);
    vm_register_native(vm, "AI.softmax",          xs_ai_softmax);
    vm_register_native(vm, "AI.crossEntropy",     xs_ai_crossEntropy);
    vm_register_native(vm, "AI.mse",              xs_ai_mse);

    /* ===== Net Module (12 functions) ===== */
    vm_register_native(vm, "Net.httpGet",    xs_net_httpGet);
    vm_register_native(vm, "Net.httpPost",   xs_net_httpPost);
    vm_register_native(vm, "Net.tcpConnect", xs_net_tcpConnect);
    vm_register_native(vm, "Net.tcpListen",  xs_net_tcpListen);
    vm_register_native(vm, "Net.tcpAccept",  xs_net_tcpAccept);
    vm_register_native(vm, "Net.tcpSend",    xs_net_tcpSend);
    vm_register_native(vm, "Net.tcpRecv",    xs_net_tcpRecv);
    vm_register_native(vm, "Net.tcpClose",   xs_net_tcpClose);
    vm_register_native(vm, "Net.udpSend",    xs_net_udpSend);
    vm_register_native(vm, "Net.udpRecv",    xs_net_udpRecv);
    vm_register_native(vm, "Net.urlEncode",  xs_net_urlEncode);
    vm_register_native(vm, "Net.urlDecode",  xs_net_urlDecode);

    /* ===== System Module (12 functions) ===== */
    vm_register_native(vm, "System.exec",        xs_sys_exec);
    vm_register_native(vm, "System.getEnv",      xs_sys_getEnv);
    vm_register_native(vm, "System.setEnv",      xs_sys_setEnv);
    vm_register_native(vm, "System.exit",        xs_sys_exit);
    vm_register_native(vm, "System.sleep",       xs_sys_sleep);
    vm_register_native(vm, "System.time",        xs_sys_time);
    vm_register_native(vm, "System.clock",       xs_sys_clock);
    vm_register_native(vm, "System.platform",    xs_sys_platform);
    vm_register_native(vm, "System.arch",        xs_sys_arch);
    vm_register_native(vm, "System.cpuCount",    xs_sys_cpuCount);
    vm_register_native(vm, "System.memoryUsage", xs_sys_memoryUsage);
    vm_register_native(vm, "System.pid",         xs_sys_pid);

    /* ===== Audio Module (11 functions) ===== */
    vm_register_native(vm, "Audio.init",             xs_audio_init);
    vm_register_native(vm, "Audio.loadSound",        xs_audio_loadSound);
    vm_register_native(vm, "Audio.playSound",        xs_audio_playSound);
    vm_register_native(vm, "Audio.stopSound",        xs_audio_stopSound);
    vm_register_native(vm, "Audio.pauseSound",       xs_audio_pauseSound);
    vm_register_native(vm, "Audio.setVolume",        xs_audio_setVolume);
    vm_register_native(vm, "Audio.setPan",           xs_audio_setPan);
    vm_register_native(vm, "Audio.createOscillator", xs_audio_createOscillator);
    vm_register_native(vm, "Audio.setFrequency",     xs_audio_setFrequency);
    vm_register_native(vm, "Audio.renderToWav",      xs_audio_renderToWav);
    vm_register_native(vm, "Audio.close",            xs_audio_close);

    /* ===== Physics Module (14 functions) ===== */
    vm_register_native(vm, "Physics.createWorld",    xs_phys_createWorld);
    vm_register_native(vm, "Physics.addBody",        xs_phys_addBody);
    vm_register_native(vm, "Physics.removeBody",     xs_phys_removeBody);
    vm_register_native(vm, "Physics.applyForce",     xs_phys_applyForce);
    vm_register_native(vm, "Physics.applyImpulse",   xs_phys_applyImpulse);
    vm_register_native(vm, "Physics.setGravity",     xs_phys_setGravity);
    vm_register_native(vm, "Physics.stepSimulation", xs_phys_stepSimulation);
    vm_register_native(vm, "Physics.raycast",        xs_phys_raycast);
    vm_register_native(vm, "Physics.checkCollision", xs_phys_checkCollision);
    vm_register_native(vm, "Physics.setMass",        xs_phys_setMass);
    vm_register_native(vm, "Physics.setFriction",    xs_phys_setFriction);
    vm_register_native(vm, "Physics.setBounce",      xs_phys_setBounce);
    vm_register_native(vm, "Physics.getPosition",    xs_phys_getPosition);
    vm_register_native(vm, "Physics.getVelocity",    xs_phys_getVelocity);

    /* ===== Crypto Module (9 functions) ===== */
    vm_register_native(vm, "Crypto.sha256",       xs_crypto_sha256);
    vm_register_native(vm, "Crypto.sha512",       xs_crypto_sha512);
    vm_register_native(vm, "Crypto.md5",          xs_crypto_md5);
    vm_register_native(vm, "Crypto.hmac",         xs_crypto_hmac);
    vm_register_native(vm, "Crypto.randomBytes",  xs_crypto_randomBytes);
    vm_register_native(vm, "Crypto.base64Encode", xs_crypto_base64Encode);
    vm_register_native(vm, "Crypto.base64Decode", xs_crypto_base64Decode);
    vm_register_native(vm, "Crypto.aesEncrypt",   xs_crypto_aesEncrypt);
    vm_register_native(vm, "Crypto.aesDecrypt",   xs_crypto_aesDecrypt);

    /* ===== Thread Module (11 functions) ===== */
    vm_register_native(vm, "Thread.spawn",       xs_thread_spawn);
    vm_register_native(vm, "Thread.join",        xs_thread_join);
    vm_register_native(vm, "Thread.detach",      xs_thread_detach);
    vm_register_native(vm, "Thread.mutexNew",    xs_thread_mutex_new);
    vm_register_native(vm, "Thread.mutexLock",   xs_thread_mutex_lock);
    vm_register_native(vm, "Thread.mutexUnlock", xs_thread_mutex_unlock);
    vm_register_native(vm, "Thread.channelNew",  xs_thread_channel_new);
    vm_register_native(vm, "Thread.channelSend", xs_thread_channel_send);
    vm_register_native(vm, "Thread.channelRecv", xs_thread_channel_recv);
    vm_register_native(vm, "Thread.atomicInc",   xs_thread_atomic_inc);
    vm_register_native(vm, "Thread.atomicDec",   xs_thread_atomic_dec);

    /* ===== Time Module (9 functions) ===== */
    vm_register_native(vm, "Time.now",         xs_time_now);
    vm_register_native(vm, "Time.timestamp",   xs_time_timestamp);
    vm_register_native(vm, "Time.format",      xs_time_format);
    vm_register_native(vm, "Time.parse",       xs_time_parse);
    vm_register_native(vm, "Time.addDuration", xs_time_addDuration);
    vm_register_native(vm, "Time.diffTime",    xs_time_diffTime);
    vm_register_native(vm, "Time.startTimer",  xs_time_startTimer);
    vm_register_native(vm, "Time.stopTimer",   xs_time_stopTimer);
    vm_register_native(vm, "Time.elapsed",     xs_time_elapsed);

    /* ===== Regex Module (6 functions) ===== */
    vm_register_native(vm, "Regex.compile",  xs_regex_compile);
    vm_register_native(vm, "Regex.match",    xs_regex_match);
    vm_register_native(vm, "Regex.matchAll", xs_regex_matchAll);
    vm_register_native(vm, "Regex.replace",  xs_regex_replace);
    vm_register_native(vm, "Regex.split",    xs_regex_split);
    vm_register_native(vm, "Regex.test",     xs_regex_test);

    /* ===== Filesystem Module (10 functions) ===== */
    vm_register_native(vm, "FS.watch",      xs_fs_watch);
    vm_register_native(vm, "FS.glob",       xs_fs_glob);
    vm_register_native(vm, "FS.realpath",   xs_fs_realpath);
    vm_register_native(vm, "FS.basename",   xs_fs_basename);
    vm_register_native(vm, "FS.dirname",    xs_fs_dirname);
    vm_register_native(vm, "FS.extname",    xs_fs_extname);
    vm_register_native(vm, "FS.joinPath",   xs_fs_joinPath);
    vm_register_native(vm, "FS.isAbsolute", xs_fs_isAbsolute);
    vm_register_native(vm, "FS.tempDir",    xs_fs_tempDir);
    vm_register_native(vm, "FS.tempFile",   xs_fs_tempFile);
}
