/*
 * Copyright 2023, Nexus6 <nexus6.haiku@icloud.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#pragma once

#include <Catalog.h>
#include <OutlineListView.h>

#include "ProjectFolder.h"
#include "SourceControlPanel.h"

enum RepositoryViewMessages {
	kUndefinedMessage,
	kInvocationMessage
};

enum ItemType {
	kLocalBranch,
	kRemoteBranch,
	kTag
};

class BranchItem;
class RepositoryView : public BOutlineListView {
public:

					 RepositoryView();
	virtual 		~RepositoryView();

	virtual void	MouseDown(BPoint where);
	virtual void	MouseMoved(BPoint point, uint32 transit, const BMessage* message);
	virtual void	AttachedToWindow();
	virtual void	DetachedFromWindow();
	virtual void	MessageReceived(BMessage* message);
	virtual void	SelectionChanged();

	// TODO: Consider returning BranchItem* directly
	BListItem*		GetSelectedItem();

	void			UpdateRepository(ProjectFolder *selectedProject, const BString &currentBranch);
private:

	void			_ShowPopupMenu(BPoint where);

	BranchItem*		_InitEmptySuperItem(const BString &label);

	BString			fRepositoryPath;
	ProjectFolder*	fSelectedProject;
	BString			fCurrentBranch;
};