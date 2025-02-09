#include "_utils.h"
#include "css.h"


const __FlashStringHelper * get_css(){
    return F("body{background:#36393e;color:#bfbfbf;font-family:sans-serif;margin:0}"
             "h1{font-size:1.7rem;margin-top:1rem;background:#2f3136;color:#bfbfbb;padding:.2em 1em;"
             "border-radius:3px;border-left:solid #20c20e 5px;font-weight:100}"
             "h2{font-size:1.1rem;margin-top:1rem;background:#2f3136;color:#bfbfbb;padding:.4em 1.8em;"
             "border-radius:3px;border-left:solid #20c20e 5px;font-weight:100}"
             "table{border-collapse:collapse}label{line-height:38px}p{margin:.5em 0}.left{float:left}"
             ".right{float:right}.bold{font-weight:700}.red{color:#F04747}"
             ".green{color:#43B581}.clear{clear:both}.centered{text-align:center}"
             ".select{width:230px!important;padding:0!important}.selected{background:#4974a9}"
             ".status{width:120px;padding-left:8px}.labelFix{line-height:44px}.clickable{cursor:pointer}"
             ".settingName{text-transform:uppercase;font-weight:700;text-decoration:underline}"
             "#status{text-align:center;text-transform:capitalize;padding:5px;color:#fff;position:sticky;top:0;z-index:99}"
             "#closeError{float:right;color:#fff;padding:0 10px;cursor:pointer}"
             "footer{font-size:.95em;text-align:center;margin-top:3em;margin-bottom:3em}"
             ".checkBoxContainer{display:block;position:relative;padding-left:35px;margin-bottom:12px;cursor:pointer;font-size:22px;-webkit-user-select:none;-moz-user-select:none;-ms-user-select:none;user-select:none;height:32px}"
             ".checkBoxContainer input{position:absolute;opacity:0;cursor:pointer}"
             ".checkmark{position:absolute;top:8px;left:0;height:28px;width:28px;background-color:#2F3136;border-radius:4px}.checkmark:after{content:'';position:absolute;display:none}.checkBoxContainer input:checked ~ .checkmark:after{display:block}.checkBoxContainer .checkmark:after{left:10px;top:7px;width:4px;height:10px;border:solid #fff;border-width:0 3px 3px 0;-webkit-transform:rotate(45deg);-ms-transform:rotate(45deg);transform:rotate(45deg)}.hide{display:none}.show{display:block!important;animation-name:fadeIn;animation-duration:1s}@keyframes fadeIn{0%{opacity:0}100%{opacity:1}}hr{background:#3e4146}a{color:#5281bb;text-decoration:none}a:hover{color:#95b8e4}li{margin:4px 0}.meter_background{background:#42464D;width:100%;word-break:normal;min-width:100px}.meter_forground{color:#fff;padding:4px 0}.meter_green{background:#43B581}.meter_orange{background:#FAA61A}.meter_red{background:#F04747}.meter_value{padding-left:8px}table{width:100%;min-width:400px;margin-bottom:2em}td{word-break:break-all}th{word-break:break-word;color:white;}th,td{padding:10px 6px;text-align:left;border-bottom:1px solid #5d5d5d}@media screen and (max-width: 820px){#apTable .id,#apTable .enc,#apTable .mac,#apTable .vendor,#apTable .name,#stTable .id,#stTable .pkts,#stTable .lastseen,#stTable .mac,#nTable .id,#nTable .vendor,#nTable .ap,#nTable .mac,#ssidTable .id{display:none}.meter_background{min-width:45px}}nav{display:block;background:#1d2236;font-weight:700;padding:0 10px}nav a{color:inherit;padding:0 .5em}.menu{list-style-type:none;margin:0;padding:0;margin:0 auto;display:flex;flex-direction:row;display:block}.menu li{margin:10px 20px 10px 0;display:inline-block}.menu li:last-child{float:right}"
             ".upload-script,.button,button,input[type='submit'],input[type='reset'],input[type='button']{display:inline-block;height:38px;padding:0 20px;color:#fff;text-align:center;font-size:11px;font-weight:600;line-height:38px;letter-spacing:.1rem;text-transform:uppercase;text-decoration:none;white-space:nowrap;background:#2f3136;border-radius:4px;border:none;cursor:pointer;box-sizing:border-box}button:hover,input[type='submit']:hover,input[type='reset']:hover,input[type='button']:hover{background:#42444a}button:active,input[type='submit']:active,input[type='reset']:active,input[type='button']:active{transform:scale(.93)}button:disabled:hover,input[type='submit']:disabled:hover,input[type='reset']:disabled:hover,input[type='button']:disabled:hover{background:#fff;cursor:not-allowed;opacity:.4;filter:alpha(opacity=40);transform:scale(1)}button::-moz-focus-inner{border:0}input[type='email'],input[type='number'],input[type='search'],input[type='text'],input[type='tel'],input[type='url'],input[type='password'],textarea,select{height:38px;padding:6px 10px;background-color:#2f3136;border-radius:4px;box-shadow:none;box-sizing:border-box;color:#d4d4d4;border:none;width:5em}input[type='text']{width:10em}"
             ".setting{width:100%!important;max-width:284px!important}input[type='file']{display:none}"
             ".container{width:100%;margin-left:auto;margin-right:auto;max-width:1140px}.row{position:relative;width:100%}.row [class^='col']{float:left;margin:.25rem 2%;min-height:.125rem}.col-1,.col-2,.col-3,.col-4,.col-5,.col-6,.col-7,.col-8,.col-9,.col-10,.col-11,.col-12{width:96%}.row::after{content:'';display:table;clear:both}.hidden-sm{display:none}"
             "@media only screen and (min-width: 24em){.col-1{width:4.33%}.col-2{width:12.66%}.col-3{width:21%}.col-4{width:29.33%}.col-5{width:37.66%}.col-6{width:46%}.col-7{width:54.33%}.col-8{width:62.66%}.col-9{width:71%}"
             ".col-10{width:79.33%}.col-11{width:87.66%}.col-12{width:96%}.hidden-sm{display:block}}.red{color:#F04747;}"
             "\n");
};

const __FlashStringHelper * get_js(){

    return F("function set_time(){\n"
            "const seconds = parseInt(new Date().getTime() / 1000);\n"
            "let options = {\n"
            "method: 'GET',\n"
            "headers: {}\n"
            "};\n"
            "fetch('/time?time=seconds', options);\n"
            "document.getElementById('dev_time').innerHTML=seconds;\n"
            "}\n");
};

const __FlashStringHelper * get_redirect(){
    return F("<script>\n"
            "var ck=4;\n"
            "var TI=null;\n"
            "function check(){"
            "const ip=document.getElementById('ip').innerHTML;\n"
            "const req='http://'+ip+'/check';"
            "fetch(req).then((data)=>{ck--;} );\n;"
            "}\n"
            "var secs=38;\n"
            "function redirx() {\n"
            "check();\n"
            "if(ck<=0)secs=0;\n"
            "secs--;\n"
            "if(secs>=0)"
            "document.getElementById('cd').innerHTML=secs+','+ck;\n"
            "if(secs==2){document.getElementById('cd').innerHTML='Try Again in a while';}\n"
            "if(secs<=0){\n"
            "clearInterval(TI);TI=null;"
            ""
            "if(ck<4) window.location.href ='/';\n"
            "else document.getElementById('cd').innerHTML='Try Again in a while';\n"
            "}}\n"
            "if(TI===null)TI=setInterval(redirx, 3000) ;\n"
            "</script>\n");
}

const __FlashStringHelper * get_logic(int index)
{
    switch(index)
    {
    default:
    case 0: return F("Disabled"); break;
    case 1: return F("Temp above and Humidity below"); break;
    case 2: return F("Temp above and Humidity above"); break;
    case 3: return F("Temp below and Humidity above"); break;
    case 4: return F("Temp below and Humidity below"); break;
    case 5: return F("Temp above OR Humidity below"); break;
    case 6: return F("Temp above OR Humidity above"); break;
    case 7: return F("Temp below OR Humidity above"); break;
    case 8: return F("Temp below OR Humidity below"); break;
    }
}
