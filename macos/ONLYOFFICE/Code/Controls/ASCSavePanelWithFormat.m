/*
 * (c) Copyright Ascensio System SIA 2010-2016
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

//
//  ASCSavePanelWithFormat.m
//  ONLYOFFICE
//
//  Created by Alexander Yuzhin on 12/11/15.
//  Copyright © 2015 Ascensio System SIA. All rights reserved.
//

#import "ASCSavePanelWithFormat.h"

@interface ASCSavePanelWithFormat()
@property (nonatomic) NSPopUpButton * popupFormats;
@end

@implementation ASCSavePanelWithFormat

+ (ASCSavePanelWithFormat *)savePanel {
    ASCSavePanelWithFormat * panel = (ASCSavePanelWithFormat *)[super savePanel];
    
    [panel initAccessoryView];
    
    return panel;
}

- (void)initAccessoryView {
    NSView  *accessoryView = [[NSView alloc] initWithFrame:NSMakeRect(0.0, 0.0, 400, 64.0)];
    
    NSTextField *label = [[NSTextField alloc] initWithFrame:NSMakeRect(0, 20, 120, 22)];
    [label setEditable:NO];
    [label setStringValue:@"File Format:"];
    [label setBordered:NO];
    [label setBezeled:NO];
    [label setDrawsBackground:NO];
    [label setAlignment:NSRightTextAlignment];
    
    self.popupFormats = [[NSPopUpButton alloc] initWithFrame:NSMakeRect(125.0, 22, 255, 22.0) pullsDown:NO];
    [self.popupFormats setAction:@selector(selectFormat:)];
    
    [accessoryView addSubview:label];
    [accessoryView addSubview:self.popupFormats];
    
    [self setAccessoryView:accessoryView];
}

- (void)selectFormat:(id)sender {
    NSPopUpButton * button = (NSPopUpButton *)sender;
    NSInteger selectedItemIndex = [button indexOfSelectedItem];
    
    _filterType = [self.filters[selectedItemIndex][@"type"] intValue];
}

- (void)setFilters:(NSArray *)filters {
    _filters = filters;
    
    for (NSDictionary * filter in filters) {
        [self.popupFormats addItemWithTitle:[NSString stringWithFormat:@"%@ (.%@)", filter[@"description"], filter[@"extension"]]];
    }
}

- (void)setFilterType:(NSInteger)filterType {
    _filterType = filterType;
    
    NSInteger index = 0;
    for (NSDictionary * filter in _filters) {
        if (_filterType == [filter[@"type"] intValue]) {
            [self.popupFormats selectItemAtIndex:index];
            break;
        }
        index++;
    }
}

@end