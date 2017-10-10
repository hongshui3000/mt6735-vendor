package com.mediatek.contacts.sanitytest.utils;

import android.content.Context;

public class ContactsTestHelper {

    private ContactsTestHelper() {
        // do nothing
    }

    public static void clearAllContacts(Context context) {
        new ContactsRemover(context).removeAllContactsBothInSimAndDb();
    }
}
