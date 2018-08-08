/*
 * (c) Copyright Ascensio System SIA 2010-2018
 *
 * This program is a free software product. You can redistribute it and/or
 * modify it under the terms of the GNU Affero General Public License (AGPL)
 * version 3 as published by the Free Software Foundation. In accordance with
 * Section 7(a) of the GNU AGPL its Section 15 shall be amended to the effect
 * that Ascensio System SIA expressly excludes the warranty of non-infringement
 * of any third-party rights.
 *
 * This program is distributed WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR  PURPOSE. For
 * details, see the GNU AGPL at: http://www.gnu.org/licenses/agpl-3.0.html
 *
 * You can contact Ascensio System SIA at Lubanas st. 125a-25, Riga, Latvia,
 * EU, LV-1021.
 *
 * The  interactive user interfaces in modified source and object code versions
 * of the Program must display Appropriate Legal Notices, as required under
 * Section 5 of the GNU AGPL version 3.
 *
 * Pursuant to Section 7(b) of the License you must retain the original Product
 * logo when distributing the program. Pursuant to Section 7(e) we decline to
 * grant you any rights under trademark law for use of our trademarks.
 *
 * All the Product's GUI elements, including illustrations and icon sets, as
 * well as technical writing content are licensed under the terms of the
 * Creative Commons Attribution-ShareAlike 4.0 International. See the License
 * terms at http://creativecommons.org/licenses/by-sa/4.0/legalcode
 *
*/


/*
*   new inherited controller declaration
*/

+function(){ 'use strict'
    var ControllerSettings = function(args={}) {
        args.caption = 'Settings';
        args.action =
        this.action = "settings";
        this.view = new ViewSettings(args);
    };

    ControllerSettings.prototype = Object.create(baseController.prototype);
    ControllerSettings.prototype.constructor = ControllerSettings;

    var ViewSettings = function(args) {
        var _lang = utils.Lang;

        args.id&&(args.id=`"id=${args.id}"`)||(args.id='');

        let _html = `<div ${args.id} class='action-panel ${args.action}'>
                        <div id='box-settings'>
                            <div class='flexbox'>
                                <h3 class='table-caption'>${_lang.actSettings}</h3>
                                <section>
                                    <div class='settings-field'>
                                        <label class='sett__caption'>${_lang.settUserName}</label>
                                        <div class='hbox' id='sett-box-user'>
                                            <input type='text' class='tbox' spellcheck='false' maxlenght='30'>
                                            <a class='link link--sizem link--gray' href='#'>${_lang.settResetUserName}</a>
                                        </div>
                                    </div>
                                    <div class='settings-field' style='display:none;'>
                                        <label class='sett__caption'>Language</label>
                                        <select>
                                            <option value='en'>English</option>
                                            <option value='ru'>Русский</option>
                                            <option value='pt_BR'>Brazil</option>
                                        </select>
                                    </div>
                                    <div class='settings-field'>
                                        <section class='switch-labeled hbox' id='sett-box-preview-mode'>
                                            <div class='onoffswitch'>
                                                <input type="checkbox" name="onoffswitch" class="onoffswitch__checkbox" id="sett-preview-mode">
                                                <label class="onoffswitch__label" for="sett-preview-mode"></label>
                                            </div>
                                            <label class='sett__caption'>${_lang.settOpenMode}</label>
                                        </section>
                                    </div>
                                </section>
                                <div class="lst-tools">
                                    <button class="btn primary" id="sett-btn-apply">${_lang.setBtnApply}</button>
                                </div>
                            </div>
                        </div>
                    </div>`;

        args.tplPage = _html;
        args.itemcls = 'bottom separate';
        args.menu = '.main-column.tool-menu';
        args.field = '.main-column.col-center';
        args.itemtext = _lang.actSettings;

        baseView.prototype.constructor.call(this, args);
    };

    ViewSettings.prototype = Object.create(baseView.prototype);
    ViewSettings.prototype.constructor = ViewSettings;

    window.ControllerSettings = ControllerSettings;

    utils.fn.extend(ControllerSettings.prototype, (function() {
        let $btnApply,
            $userName,
            $chOpenMode;

        function _set_user_name(name) {
            let me = this;

            $userName.val(name).removeClass('error');
            $btnApply.prop('disabled', false);
        };

        function _on_btn_apply(e) {
            let _user_new_name = $userName.val();
            if ( _user_new_name && _user_new_name.length ) {
                let _doc_open_mode = $chOpenMode.prop('checked') ? 'view' : 'edit';

                sdk.command("settings:apply", JSON.stringify({username:_user_new_name, docopenmode: _doc_open_mode}));
                $btnApply.prop('disabled', true);
                
                localStorage.setItem('username', _user_new_name);
                localStorage.setItem('docopenmode', _doc_open_mode);
            } else {
                $userName.addClass('error');
            }
        };

        function _on_txt_user_change(e) {
            $userName.removeClass('error');
            
            if ( $btnApply.prop('disabled') )
                $btnApply.prop('disabled', false);
        };

        function _lock_createnew(lock) {
            lock === true ? $('.tool-quick-menu .menu-item').addClass('disabled') :
                    $('.tool-quick-menu .menu-item').removeClass('disabled');
        };

        return {
            init: function() {
                baseController.prototype.init.apply(this, arguments);

                this.view.render();

                let me = this;
                me.view.$panel.find('#sett-box-user > a.link').on('click', e => {
                    sdk.command("settings:get", "username");
                });

                $btnApply = me.view.$panel.find('#sett-btn-apply');
                $userName = me.view.$panel.find('#sett-box-user > input');
                $chOpenMode = me.view.$panel.find('#sett-preview-mode');

                $btnApply.on('click', _on_btn_apply).prop('disabled', true);
                $userName.on('keypress', _on_txt_user_change);
                $chOpenMode.on('change', e => {
                    if ( $btnApply.prop('disabled') )
                        $btnApply.prop('disabled', false);

                    _lock_createnew($chOpenMode.prop('checked'));
                });

                let _user_name = localStorage.getItem('username') || '';
                let _open_mode = localStorage.getItem('docopenmode') || 'edit';

                if ( _user_name ) {
                    $userName.val(_user_name);
                } else {
                    sdk.command("settings:get", "username");
                }

                if ( _open_mode == 'view' ) {
                    $chOpenMode.prop('checked', true);
                    _lock_createnew(true);
                }

                if ( _user_name || _open_mode ) {
                    sdk.command("settings:apply", JSON.stringify({username:_user_name, docopenmode: _open_mode}));
                }

                window.sdk.on('on_native_message', (cmd, param) => {
                    if (/settings\:username/.test(cmd)) {
                        _set_user_name.call(this, param);
                    }
                });

                return this;
            }
        };
    })());
}();

/*
*   controller definition
*/

// window.CommonEvents.on('main:ready', function(){
//     var p = new ControllerSettings({});
//     p.init();
// });