rootkey {
    subkey {
        key = "overwrite";
        child = "value";
        mergekey = "new";
    }
    array [
        "a",
        "b",
        "c",
        "d",
        "e",
        "f",
        [
            1,
            2,
            3,
        ]
    ]
    newarray [
        "apple",
        "orange",
    ]
}
newroot {
    subkey {
        key = "new";
    }
}
